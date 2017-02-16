/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsClientCache.hpp"
#include "NewsClientConnect.hpp"
#include "NewsClientReadArticleStream.hpp"
#include "NewsClientStatArticle.hpp"
#include "NewsClientReadOverview.hpp"

namespace ByteFountain
{

	template <typename T, typename... TArgs>
	static void SendEvent(T& handler, TArgs&&... args)
	{
		auto h = handler;
		if (h) h(std::forward<TArgs>(args)...);
	}


	struct NewsClientCache::MyServer : public NewsServer
	{
		int32_t serverID;
		NewsServerPriority priority;
		std::set<std::string> groupBlacklist;

		MyServer(int32_t sid, const NewsServerPriority pr, const NewsServer& server) :
			NewsServer(server), serverID(sid), priority(pr), groupBlacklist()
		{}

		// Blacklist group if they are not supported on server:

		void AddToGroupBlacklist(const std::string& group)
		{
			groupBlacklist.insert(group);
		}

		std::vector<std::string> FilterBlacklistedGroups(const std::vector<std::string>& groups)
		{
			std::vector<std::string> res;
			for (const auto& group : groups)
			{
				if (groupBlacklist.count(group) == 0) res.push_back(group);
			}
			return res;
		}

	};

	bool NewsClientCache::ServerOrder::operator()(const ServerPtr& a, const ServerPtr& b) const
	{
		if (a->priority < b->priority) return true;
		if (a->priority > b->priority) return false;

		return a->serverID < b->serverID;
	}

	struct NewsClientCache::MyClient : public NewsClient, std::enable_shared_from_this < MyClient >
	{
		ServerPtr server;
		const int32_t clientID;
		RateLimiter& limiter;
		std::queue< CommandPtr > commands;

		MyClient(const int cid, io_service& ios, Logger& logger, RateLimiter& lim,
			const ServerPtr& srv, NewsClient::DoneHandler done) :
			NewsClient(ios, logger, lim, *srv, done), server(srv), clientID(cid), limiter(lim), commands()
		{
		}

		~MyClient()
		{
		}

		std::shared_ptr<MyClient> GetSharedPtr()
		{
			return shared_from_this();
		}

		bool HasCapacity() const; // declared later

		std::ostream& print(std::ostream& log) const
		{
			log << "Client (" << server->serverID << "," << clientID << "," << commands.size() << ")";
			return log;
		}

		std::function< void(const std::shared_ptr<MyClient>& client) > ClientStarted;

		virtual void OnStarted()
		{
			auto handler = ClientStarted;
			if (handler) handler(GetSharedPtr());
		}

		std::function< void(const std::shared_ptr<MyClient>& client) > ClientStopped;

		virtual void OnStopped()
		{
			auto handler = ClientStopped;
			if (handler) handler(GetSharedPtr());
		}

		virtual void NoSuchGroupOnServer(const std::string& group)
		{
			server->AddToGroupBlacklist(group);
		}

		virtual std::vector<std::string> FilterBlacklistedGroups(const std::vector<std::string>& groups)
		{
			return server->FilterBlacklistedGroups(groups);
		}

	};

	std::ostream& operator<<(std::ostream& log, const NewsClientCache::MyClient& client)
	{
		return client.print(log);
	}

	bool NewsClientCache::ClientOrder::operator()(const ClientPtr& a, const ClientPtr& b) const
	{
		return a->clientID < b->clientID;
	}

	struct NewsClientCache::MyCommand : std::enable_shared_from_this < MyCommand >
	{
		uint_fast64_t m_commandID;
		ConCancelScoped m_cancel;
		std::set<int> m_failed;
		std::set<int> m_timeout;
		bool m_canceled;
		std::atomic<int32_t>& m_priorityCommands;
		bool m_priority;

		MyCommand(const uint_fast64_t cid, SigCancel* cancel, std::atomic<int32_t>& priorityCommands, const bool priority) :
			m_commandID(cid),
			m_cancel(),
			m_failed(),
			m_canceled(false),
			m_priorityCommands(priorityCommands),
			m_priority(priority)
		{
			if (cancel != 0) m_cancel = cancel->connect([this](){ Cancel(); });
			if (m_priority) m_priorityCommands++;
		}

		virtual ~MyCommand()
		{
			if (m_priority) m_priorityCommands--;
		}

		std::shared_ptr<MyCommand> GetSharedPtr()
		{
			return shared_from_this();
		}

		void AddFailed(const ClientPtr& cli)
		{
			m_failed.insert(cli->server->serverID);
		}
		void AddTimeout(const ClientPtr& cli)
		{
			m_timeout.insert(cli->server->serverID);
		}
		void ClearTimeout()
		{
			m_timeout.clear();
		}

		bool HasFailed(const ServerPtr& srv) const
		{
			return m_failed.find(srv->serverID) != m_failed.end();
		}
		bool HasTimeout(const ServerPtr& srv) const
		{
			return m_timeout.find(srv->serverID) != m_timeout.end();
		}

		virtual bool Pipeable() const = 0;

		virtual void Cancel() { m_canceled = true; }

		void Run(MyClient& client)
		{
			client.commands.push(shared_from_this());
			InternalRun(client);
		}

		void Completed(MyClient& client, const NewsClient::DoneCode& code)
		{
			client.commands.pop();
		}

	private:

		virtual void InternalRun(MyClient& client) = 0;
	};

	bool NewsClientCache::MyClient::HasCapacity() const
	{
		return commands.size() < server->Pipeline && (commands.size() != 1 || commands.front()->Pipeable());
	}


	template <typename TCmd> struct NewsClientCache::MyCommandT : public TCmd, public MyCommand
	{
		template <typename... TArgs>
		MyCommandT(const uint_fast64_t cid, SigCancel* cancel, std::atomic<int32_t>& priorityCommands, const bool priority, TArgs&&... args) :
			TCmd(std::forward<TArgs>(args)...),
			MyCommand(cid, cancel, priorityCommands, priority)
		{
		}

		virtual ~MyCommandT(){}

		virtual bool Pipeable() const { return TCmd::Pipeable(); }
		void InternalRun(MyClient& client) { TCmd::Run(client); }
		void Cancel() { TCmd::OnCommandCancel(); MyCommand::Cancel(); }
	};

	void NewsClientCache::SendEventQueueEmptyEvent()
	{
		m_strand.post([this]() // TODO: Try making an event-strand
		{
			auto handler = m_onWaitingQueueEmpty;
			if (handler) handler();
		}
		);
	}

	void NewsClientCache::SendConnectionInfoEvents(const int64_t time)
	{
		m_strand.post([time, this]()
		{
			for (const auto& kv : m_connections)
			{
				const ServerPtr& sp(kv.first);
				SendEvent(m_connectionInfoFunction, time, sp->serverID, sp->Statistics.BytesTX, sp->Statistics.BytesRX,
					sp->Statistics.MissingSegmentCount, sp->Statistics.ConnectionTimeoutCount, sp->Statistics.ErrorCount);
			}
		});
	}

	void NewsClientCache::RunCommand(ClientPtr& client, CommandPtr& command)
	{
		//		if (m_commandsRunning.size()==0) SendInfoEvent(Starting);

		m_logger << Logger::Debug << *client << ": Running command #" << command->m_commandID << " commands waiting=" << m_commandsWaiting.size() << Logger::End;

		m_commandsRunning.insert(command);
		command->Run(*client);
	}

	void NewsClientCache::DispatchCommands() // RUN UNDER m_strand
	{
		if (m_connections.size() == 0 || m_commandsWaiting.size() == 0) return;

		//		m_logger<<Logger::Debug<<"Dispatching commands..."<<Logger::End;

		for (auto itCmd = m_commandsWaiting.begin(); itCmd != m_commandsWaiting.end(); /*nothing*/)
		{
			CommandPtr cmd = *itCmd;
			if (cmd->m_canceled)
			{
				m_logger << Logger::Debug << "Command #" << cmd->m_commandID << " was canceled" << Logger::End;
				itCmd = m_commandsWaiting.erase(itCmd);
			}
			else
			{
				ClientPtr cli;
				bool failedAllServers = true;
				bool timeoutAllServers = true;
				NewsServerPriority priority = NewsServerPriority::Primary;

				for (auto itConn = m_connections.begin(); !cli && itConn != m_connections.end(); itConn++)
				{
					ServerPtr srv = itConn->first;

					if (priority != srv->priority)
					{
						if (!failedAllServers) break;
						if (!timeoutAllServers) break;
						priority = srv->priority;
					}

					if (cmd->HasFailed(srv)) continue;

					failedAllServers = false;

					if (cmd->HasTimeout(srv)) continue;

					timeoutAllServers = false;

					std::set<ClientPtr, ClientOrder>& clients(itConn->second);

					for (auto itCli = clients.begin(); !cli && itCli != clients.end(); itCli++)
					{
						if ((*itCli)->HasCapacity())
						{
							cli = *itCli;
						}
					}
				}
				if (cli)
				{
					SendEvent(m_connectionStateChangedFunction, ConnectionState::Working, cli->server->serverID, cli->clientID);
					RunCommand(cli, cmd);
					itCmd = m_commandsWaiting.erase(itCmd);
				}
				else
				{
					if (failedAllServers)
					{
						m_logger << Logger::Error << "Command #" << cmd->m_commandID << " failed on all servers" << Logger::End;
						itCmd = m_commandsWaiting.erase(itCmd);
					}
					else
					{
						if (timeoutAllServers) cmd->ClearTimeout(); // Try again
						++itCmd;
					}
				}
			}
		}

		if (m_commandsWaiting.empty()) SendEventQueueEmptyEvent();
	}

	void NewsClientCache::SetWaitingQueueEmptyHandler(const std::function< void() >& handler)
	{
		m_onWaitingQueueEmpty = handler;

		m_strand.dispatch([this]()
		{
			if (m_commandsWaiting.size() == 0)
			{
				auto fn = m_onWaitingQueueEmpty;
				if (fn) fn();
			}
		});
	}

	void NewsClientCache::SetConnectionStateChangedHandler(ConnectionStateChangedFunction handler)
	{
		m_connectionStateChangedFunction = handler;
	}

	void NewsClientCache::SetConnectionInfoHandler(ConnectionInfoFunction handler)
	{
		m_connectionInfoFunction = handler;
	}

	void NewsClientCache::StartClient(const int32_t clientID, const ServerPtr& srv)
	{
		ClientPtr client(
			new MyClient(clientID, m_ios, m_logger, m_limiter, srv,
			[this](NewsClient& client, NewsClientCommand& command, const NewsClient::DoneCode& code)
		{
			ClientPtr myclient = static_cast<MyClient&>(client).GetSharedPtr();
			CommandPtr mycommand = dynamic_cast<MyCommand&>(command).GetSharedPtr();
			mycommand->Completed(*myclient, code);
			ClientDone(myclient, mycommand, code);
		}
		));

		CommandPtr command(
			new MyCommandT<NewsClientConnect>(++m_commandCounter, nullptr, m_priorityCommands, true, m_ios, m_logger)
			);

		SendEvent(m_connectionStateChangedFunction, ConnectionState::Connecting, srv->serverID, client->clientID);

		m_connections[srv].insert(client);
		RunCommand(client, command);

	}

	int32_t NewsClientCache::AddServer(const NewsServer& server, const NewsServerPriority priority, const int32_t threads)
	{
		int32_t sid = m_serverCounter++;

		m_strand.post([this, sid, priority, server, threads]()
		{
			ServerPtr srv(new MyServer(sid, priority, server));

			m_logger << Logger::Info << "Connecting to server #" << sid << ": " << *srv << "..." << Logger::End;

			for (int32_t i = 0; i < threads; ++i)
			{
				StartClient(i, srv);
			}
		});

		return sid;
	}


	void NewsClientCache::StopClient(const ClientPtr& cli)
	{
		if (m_connections[cli->server].erase(cli) != 0) // Move connection to bin.
		{
			if (m_connections[cli->server].empty())
			{
				m_connections.erase(cli->server);
			}

			m_logger << Logger::Debug << *cli << ": Stopping client" << Logger::End;

			m_connectionsBin.insert(cli);
			cli->ClientStopped = [this](const ClientPtr& cli)
			{
				m_logger << Logger::Debug << *cli << ": Client stopped" << Logger::End;

				m_connectionsBin.erase(cli);
			};
		}

		SendEvent(m_connectionStateChangedFunction, ConnectionState::Disconnected, cli->server->serverID, cli->clientID);
	}

	void NewsClientCache::RestartClient(const ClientPtr& cli)
	{
		if (m_connections[cli->server].erase(cli) != 0) // Move connection to bin.
		{
			m_logger << Logger::Debug << *cli << ": Stopping client" << Logger::End;

			m_connectionsBin.insert(cli);
			cli->ClientStopped = [this](const ClientPtr& cli)
			{
				m_logger << Logger::Debug << *cli << ": Client restarting" << Logger::End;

				m_connectionsBin.erase(cli);
				StartClient(cli->clientID, cli->server);
			};
		}
	}

	void NewsClientCache::ClientDone(const ClientPtr& myclient, const CommandPtr& mycommand, const NewsClient::DoneCode& code)
	{
		m_commandsRunning.erase(mycommand);

		m_logger << Logger::Debug << *myclient << ": Command #" << mycommand->m_commandID << " ended (" << code << ")" << Logger::End;

		if (code == NewsClient::DoneCode::DisconnectClient)
		{
			SendEvent(m_connectionStateChangedFunction, ConnectionState::Disconnected, myclient->server->serverID, myclient->clientID);
			m_logger << Logger::Debug << *myclient << ": Disconnecting Client" << Logger::End;

			StopClient(myclient);
			myclient->Disconnect();
		}
		else if (code == NewsClient::DoneCode::Error)
		{
			SendEvent(m_connectionStateChangedFunction, ConnectionState::Idle, myclient->server->serverID, myclient->clientID);

			mycommand->AddFailed(myclient);
			m_commandsWaiting.emplace_front(mycommand);
		}
		else if (code == NewsClient::DoneCode::RetryOther)
		{
			SendEvent(m_connectionStateChangedFunction, ConnectionState::Idle, myclient->server->serverID, myclient->clientID);

			mycommand->AddFailed(myclient);
			m_commandsWaiting.emplace_front(mycommand);
		}
		else if (code == NewsClient::DoneCode::ReconnectRetry || !myclient->IsConnected())
		{
			SendEvent(m_connectionStateChangedFunction, ConnectionState::Idle, myclient->server->serverID, myclient->clientID);

			m_logger << Logger::Debug << *myclient << ": Reconnecting Client" << Logger::End;

			mycommand->AddTimeout(myclient);
			m_commandsWaiting.emplace_front(mycommand);

			RestartClient(myclient);
		}
		else
		{
			SendEvent(m_connectionStateChangedFunction, ConnectionState::Idle, myclient->server->serverID, myclient->clientID);
		}


		DispatchCommands();
	}

	NewsClientCache::NewsClientCache(io_service& ios, Logger& logger, RateLimiter& limiter) :
		m_ios(ios), m_strand(m_ios), m_logger(logger), m_defaultLimiter(ios, logger),
		m_limiter(limiter), m_serverCounter(0), m_commandCounter(0),
		m_onWaitingQueueEmpty(), m_connectionStateChangedFunction(),
		m_connections(), m_commandsWaiting(), m_commandsRunning(), m_priorityCommands(0)
	{}
	NewsClientCache::NewsClientCache(io_service& ios, Logger& logger) :
		m_ios(ios), m_strand(m_ios), m_logger(logger), m_defaultLimiter(ios, logger),
		m_limiter(m_defaultLimiter), m_serverCounter(0), m_commandCounter(0),
		m_onWaitingQueueEmpty(), m_connectionStateChangedFunction(),
		m_connections(), m_commandsWaiting(), m_commandsRunning(), m_priorityCommands(0)
	{}
	NewsClientCache::~NewsClientCache()
	{
	}

	size_t NewsClientCache::WaitingQueueSize() const
	{
		return m_commandsWaiting.size(); // TODO: make threadsafe by using atomic value.
	}

	void NewsClientCache::PostCommand(MyCommand* command)
	{
		CommandPtr cmd(command);

		m_logger << Logger::Debug << "Posting " << (command->m_priority ? "priority" : "") << " command #" << cmd->m_commandID << ": " << dynamic_cast<const NewsClientCommand&>(*cmd) << Logger::End;

		m_strand.dispatch([this, cmd]()
		{
			m_commandsWaiting.emplace_back(cmd);
			DispatchCommands();
		});
	}

	std::future<void> NewsClientCache::AsyncStop()
	{
		auto promise = std::make_shared< std::promise<void> >();

		m_strand.post([this, promise]() mutable
		{
			for (auto it = m_commandsWaiting.begin(); it != m_commandsWaiting.end();)
			{
				const CommandPtr& cmd(*it);
				m_logger << Logger::Debug << "Canceling command #" << cmd->m_commandID << Logger::End;
				cmd->Cancel();
				it = m_commandsWaiting.erase(it);
			}
			for (auto it = m_commandsRunning.begin(); it != m_commandsRunning.end(); ++it)
			{
				const CommandPtr& cmd(*it);
				m_logger << Logger::Debug << "Canceling command #" << cmd->m_commandID << Logger::End;
				cmd->Cancel();
			}

			std::list<ClientPtr> disconnecting;

			for (auto conn : m_connections)
			{
				for (auto cli : conn.second) disconnecting.push_back(cli);
			}

			m_connections.clear();

			AsyncDisconnectClients(disconnecting, promise);
		});

		return promise->get_future();
	}


	void NewsClientCache::Stop()
	{
		m_logger<<Logger::Info<<"NewsClientCache Stopping."<<Logger::End;
		
		std::future<void> future = AsyncStop();

		future.get();

		m_logger<<Logger::Info<<"NewsClientCache Stopped."<<Logger::End;

	}


	void NewsClientCache::RemoveServer(const int32_t id)
	{
		auto promise = std::make_shared<std::promise<void>>();

		m_strand.post([this, id, promise]()
		{

			std::list<ClientPtr> disconnecting;

			for (auto conn : m_connections)
			{
				if (conn.first->serverID == id)
				{
					for (auto cli : conn.second) disconnecting.push_back(cli);
					m_connections.erase(conn.first);
					break;
				}
			}

			AsyncDisconnectClients(disconnecting, promise);

		});

		std::future<void> future = promise->get_future();
		future.get();
	}


	void NewsClientCache::AsyncDisconnectClients(std::list<NewsClientCache::ClientPtr> clients, std::shared_ptr< std::promise<void> > promise)
	{
		m_connectionsBin.insert(clients.begin(), clients.end());

		if (clients.empty())
		{
			promise->set_value();
		}
		else
		{
			for (auto itCli = clients.begin(); itCli != clients.end(); itCli++)
			{
				const ClientPtr& cli(*itCli);

				cli->ClientStopped = [this, promise](const ClientPtr& cli)
				{
					m_logger << Logger::Debug << *cli << ": Client stopped" << Logger::End;

					m_connectionsBin.erase(cli);

					if (m_connectionsBin.empty())
					{
						m_logger << Logger::Info << "All connections closed." << Logger::End;
						promise->set_value();
					}

				};

				cli->Disconnect();
			}
		}
	}

/*******************************************************	
Info: FINISHED: alt.binaries.sounds.rar.mp3
Info: HORIZON FILTER COUNT=0
Info: Work completed
Info: NewsClientCache Stopping.
Debug: Client (1,0,0): Client stopped
Debug: Client (2,0,0): Client stopped
Debug: Client (3,0,0): Client stopped
Debug: Client (4,0,0): Client stopped
Info: All connections closed.
Info: NewsClientCache Stopped.
==7583== Thread 2:
==7583== Invalid read of size 4
==7583==    at 0x4E44A91: pthread_once (pthread_once.S:111)
==7583==    by 0x45CE02: __gthread_once(int*, void (*)()) (gthr-default.h:699)
==7583==    by 0x46A758: void std::call_once<void (std::__future_base::_State_base::*)(std::function<std::unique_ptr<std::__future_base::_Result_base, std::__future_base::_Result_base::_Deleter> ()>&, bool&), std::__future_base::_State_base* const, std::reference_wrapper<std::function<std::unique_ptr<std::__future_base::_Result_base, std::__future_base::_Result_base::_Deleter> ()> >, std::reference_wrapper<bool> >(std::once_flag&, void (std::__future_base::_State_base::*&&)(std::function<std::unique_ptr<std::__future_base::_Result_base, std::__future_base::_Result_base::_Deleter> ()>&, bool&), std::__future_base::_State_base* const&&, std::reference_wrapper<std::function<std::unique_ptr<std::__future_base::_Result_base, std::__future_base::_Result_base::_Deleter> ()> >&&, std::reference_wrapper<bool>&&) (mutex:786)
==7583==    by 0x467D84: std::__future_base::_State_base::_M_set_result(std::function<std::unique_ptr<std::__future_base::_Result_base, std::__future_base::_Result_base::_Deleter> ()>, bool) (future:358)
==7583==    by 0x46CEA9: std::promise<bool>::set_value(bool&&) (future:994)
==7583==    by 0x45F460: ByteFountain::NewsClientCache::Stop()::{lambda()#1}::operator()()::{lambda(std::shared_ptr<ByteFountain::NewsClientCache::MyClient> const&)#1}::operator()(std::shared_ptr<ByteFountain::NewsClientCache::MyClient> const) const (NewsClientCache.cpp:494)
==7583==    by 0x4624C5: std::_Function_handler<void (std::shared_ptr<ByteFountain::NewsClientCache::MyClient> const&), ByteFountain::NewsClientCache::Stop()::{lambda()#1}::operator()()::{lambda(std::shared_ptr<ByteFountain::NewsClientCache::MyClient> const&)#1}>::_M_invoke(std::_Any_data const&, std::shared_ptr<ByteFountain::NewsClientCache::MyClient> const&) (functional:2071)
==7583==    by 0x46ADF2: std::function<void (std::shared_ptr<ByteFountain::NewsClientCache::MyClient> const&)>::operator()(std::shared_ptr<ByteFountain::NewsClientCache::MyClient> const&) const (functional:2464)
==7583==    by 0x4687A5: ByteFountain::NewsClientCache::MyClient::OnStopped() (NewsClientCache.cpp:74)
==7583==    by 0x448637: ByteFountain::NewsClient::Disconnect() (in /home/stauning/ByteFountain)
==7583==    by 0x45F9F5: ByteFountain::NewsClientCache::Stop()::{lambda()#1}::operator()() (NewsClientCache.cpp:499)
==7583==    by 0x463904: void boost::asio::asio_handler_invoke<ByteFountain::NewsClientCache::Stop()::{lambda()#1}>(ByteFountain::NewsClientCache::Stop()::{lambda()#1}, ...) (handler_invoke_hook.hpp:64)
==7583==  Address 0x12dacc14 is 132 bytes inside a block of size 136 free'd
==7583==    at 0x4C2C2BC: operator delete(void*) (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==7583==    by 0x47E691: __gnu_cxx::new_allocator<std::_Sp_counted_ptr_inplace<std::__future_base::_State_base, std::allocator<std::__future_base::_State_base>, (__gnu_cxx::_Lock_policy)2> >::deallocate(std::_Sp_counted_ptr_inplace<std::__future_base::_State_base, std::allocator<std::__future_base::_State_base>, (__gnu_cxx::_Lock_policy)2>*, unsigned long) (new_allocator.h:110)
==7583==    by 0x47CDD0: std::allocator_traits<std::allocator<std::_Sp_counted_ptr_inplace<std::__future_base::_State_base, std::allocator<std::__future_base::_State_base>, (__gnu_cxx::_Lock_policy)2> > >::deallocate(std::allocator<std::_Sp_counted_ptr_inplace<std::__future_base::_State_base, std::allocator<std::__future_base::_State_base>, (__gnu_cxx::_Lock_policy)2> >&, std::_Sp_counted_ptr_inplace<std::__future_base::_State_base, std::allocator<std::__future_base::_State_base>, (__gnu_cxx::_Lock_policy)2>*, unsigned long) (alloc_traits.h:377)
==7583==    by 0x485111: std::_Sp_counted_ptr_inplace<std::__future_base::_State_base, std::allocator<std::__future_base::_State_base>, (__gnu_cxx::_Lock_policy)2>::_M_destroy() (shared_ptr_base.h:417)
==7583==    by 0x44E44F: std::_Sp_counted_base<(__gnu_cxx::_Lock_policy)2>::_M_release() (shared_ptr_base.h:161)
==7583==    by 0x44AD5A: std::__shared_count<(__gnu_cxx::_Lock_policy)2>::~__shared_count() (shared_ptr_base.h:546)
==7583==    by 0x468011: std::__shared_ptr<std::__future_base::_State_base, (__gnu_cxx::_Lock_policy)2>::~__shared_ptr() (shared_ptr_base.h:781)
==7583==    by 0x468031: std::shared_ptr<std::__future_base::_State_base>::~shared_ptr() (shared_ptr.h:93)
==7583==    by 0x46CB85: std::promise<bool>::~promise() (future:958)
==7583==    by 0x45FC33: ByteFountain::NewsClientCache::Stop() (NewsClientCache.cpp:507)
==7583==    by 0x616742: ByteFountain::NZBDrive::Stop() (NZBDrive.cpp:182)
==7583==    by 0x487E70: main (ByteFountain.cpp:625)
==7583== 
Info: Drive stopped

*////////////////////////////////////

	void NewsClientCache::CancelCommand(uint_fast64_t cid)
	{
		m_strand.post([this,cid]()
		{
			for(auto it = m_commandsWaiting.begin(); it != m_commandsWaiting.end(); ++it)
			{
				const CommandPtr& cmd(*it);
				if (cmd->m_commandID == cid)
				{
					m_logger<<Logger::Debug<<"Canceling command #"<<cmd->m_commandID<<Logger::End;
					cmd->Cancel();
					m_commandsWaiting.erase(it);
					return;
				}
			}
			for(auto it = m_commandsRunning.begin(); it != m_commandsRunning.end(); ++it)
			{
				const CommandPtr& cmd(*it);
				if (cmd->m_commandID == cid)
				{
					m_logger<<Logger::Debug<<"Canceling command #"<<cmd->m_commandID<<Logger::End;
					cmd->Cancel();
					return;
				}
			}
		});
	}
	
	void NewsClientCache::GetArticleStream(const std::vector<std::string>& groups, const std::vector<std::string>& msgIDs,
			      std::shared_ptr<IArticleStreamHandler> handler, const bool priority)
	{
		PostCommand( new MyCommandT<NewsClientReadArticleStream>
			(++m_commandCounter, nullptr, m_priorityCommands, priority, m_ios, m_logger, groups, msgIDs, handler)
		);
	}
	
	void NewsClientCache::GetArticleStream(const std::vector<std::string>& groups, const std::vector<std::string>& msgIDs,
			      std::shared_ptr<IArticleStreamHandler> handler, SigCancel& cancel, const bool priority)
	{
		PostCommand( new MyCommandT<NewsClientReadArticleStream>
			(++m_commandCounter, &cancel, m_priorityCommands, priority, m_ios, m_logger, groups, msgIDs, handler)
		);
	}
	
	void NewsClientCache::StatArticle(const std::string& group, const std::string& msgID, 
			      std::function<void (const bool succeeded)> handler)
	{
		PostCommand( new MyCommandT<NewsClientStatArticle>
			(++m_commandCounter, nullptr, m_priorityCommands, true, m_ios, m_logger, group, msgID, handler)
		);
	}
	void NewsClientCache::StatArticle(const std::string& group, const std::string& msgID, 
			      std::function<void (const bool succeeded)> handler, SigCancel& cancel)
	{
		PostCommand( new MyCommandT<NewsClientStatArticle>
			(++m_commandCounter, &cancel, m_priorityCommands, true, m_ios, m_logger, group, msgID, handler)
		);
	}
	
	void NewsClientCache::GetOverviewStream(const std::string& group, 
		const unsigned long long fromID, const unsigned long long toID, 
		std::shared_ptr<IOverviewResponseHandler> handler)
	{
		PostCommand( new MyCommandT<NewsClientReadOverview>
			(++m_commandCounter, nullptr, m_priorityCommands, true, m_ios, m_logger, group, fromID, toID, handler)
		);
	}
	
	void NewsClientCache::GetOverviewStream(const std::string& group, 
						std::shared_ptr<IOverviewResponseHandler> handler)
	{
		PostCommand( new MyCommandT<NewsClientReadOverview>
			(++m_commandCounter, nullptr, m_priorityCommands, true, m_ios, m_logger, group, 0,
			 std::numeric_limits<unsigned long long>::max(), handler)
		);
	}


}


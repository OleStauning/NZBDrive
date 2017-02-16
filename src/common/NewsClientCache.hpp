/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#ifndef NEWSCLIENTCACHE_HPP
#define NEWSCLIENTCACHE_HPP

#include "Logger.hpp"
#include "NewsClient.hpp"
#include "NewsClientCommand.hpp"
#include "IArticleStreamHandler.hpp"
#include "IOverviewResponseHandler.hpp"
#include "ConnectionState.hpp"
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <atomic>
#include <queue>
#include <future>
#include <set>
#include <unordered_set>
#include <functional>

namespace ByteFountain
{

typedef boost::asio::io_service io_service;


enum class NewsServerPriority { Primary, Backup };


class NewsClientCache
{
public:
	typedef boost::signals2::signal<void ()> SigCancel;

	typedef std::function< void(const ConnectionState state, const int32_t server, const int32_t thread) > ConnectionStateChangedFunction;
	typedef std::function< void(const int64_t time, const int32_t server, const uint64_t bytesTX, uint64_t bytesRX, 
		uint32_t missingSegmentCount, uint32_t connectionTimeoutCount, uint32_t errorCount)> ConnectionInfoFunction;

	struct MyServer;
	typedef std::shared_ptr<MyServer> ServerPtr;

	struct MyClient;
	typedef std::shared_ptr<MyClient> ClientPtr;
	
	struct MyCommand;
	template <typename TCmd> struct MyCommandT;
	typedef std::shared_ptr<MyCommand> CommandPtr;
	
	struct ServerOrder { bool operator()( const ServerPtr& a, const ServerPtr& b ) const; };
	struct ClientOrder { bool operator()( const ClientPtr& a, const ClientPtr& b ) const; };

private:
	typedef boost::signals2::connection ConCancel;
	typedef boost::signals2::scoped_connection ConCancelScoped;

	io_service& m_ios;
	io_service::strand m_strand;
	Logger& m_logger;
	RateLimiter m_defaultLimiter;
	RateLimiter& m_limiter;
	
	std::atomic<int32_t> m_serverCounter;
	std::atomic<uint_fast64_t> m_commandCounter;

	std::function< void() > m_onWaitingQueueEmpty;
	ConnectionStateChangedFunction m_connectionStateChangedFunction;
	ConnectionInfoFunction m_connectionInfoFunction;
	
	std::map< ServerPtr, std::set<ClientPtr, ClientOrder>, ServerOrder > m_connections;
	std::set< ClientPtr > m_connectionsBin;
	std::deque< CommandPtr > m_commandsWaiting;
	std::set< CommandPtr > m_commandsRunning;

	std::atomic<int32_t> m_priorityCommands;
	
//	void RescheduleClientCommands(ClientPtr client);
	
	void RunCommand(ClientPtr& client, CommandPtr& command);
	
	void DispatchCommands();
	void StartClient(const int32_t clientID, const ServerPtr& srv);
	void StopClient(const ClientPtr& cli);
	void RestartClient(const ClientPtr& cli);
	void ClientDone(const ClientPtr& myclient, const CommandPtr& mycommand, const NewsClient::DoneCode& code);

	void PostCommand(MyCommand* command);
//	void PostCommand(NewsClientCommand* cmd, SigCancel* cancel);

	void CancelCommand(uint_fast64_t cid);

	void SendEventQueueEmptyEvent();

	void AsyncDisconnectClients(std::list<NewsClientCache::ClientPtr> clients, std::shared_ptr< std::promise<void> > promise);
	
public:

	NewsClientCache(io_service& ios, Logger& logger, RateLimiter& limiter);
	NewsClientCache(io_service& ios, Logger& logger);
	~NewsClientCache();
	
	std::future<void> AsyncStop();
	void Stop();

	bool HasPriorityWork() const { return m_priorityCommands > 0; }

	void SetWaitingQueueEmptyHandler(const std::function< void() >& handler);

	void SetConnectionStateChangedHandler(ConnectionStateChangedFunction handler);
	void SetConnectionInfoHandler(ConnectionInfoFunction handler);

	void SendConnectionInfoEvents(const int64_t time);

	size_t WaitingQueueSize() const;

	int32_t AddServer(const NewsServer& server, const NewsServerPriority priority, const int32_t threads);
	void RemoveServer(const int32_t id);
		
	void GetArticleStream(const std::vector<std::string>& groups, const std::vector<std::string>& msgIDs,
			      std::shared_ptr<IArticleStreamHandler> handler, const bool priority=false);
	void GetArticleStream(const std::vector<std::string>& groups, const std::vector<std::string>& msgIDs,
		std::shared_ptr<IArticleStreamHandler> handler, SigCancel& cancel, const bool priority = false);

	void StatArticle(const std::string& group, const std::string& msgID, 
			      std::function<void (const bool succeeded)> handler);
	void StatArticle(const std::string& group, const std::string& msgID, 
			      std::function<void (const bool succeeded)> handler, SigCancel& cancel);
	
	void GetOverviewStream(const std::string& group, const unsigned long long fromID, const unsigned long long toID, 
			      std::shared_ptr<IOverviewResponseHandler> handler);
	void GetOverviewStream(const std::string& group, std::shared_ptr<IOverviewResponseHandler> handler);
};


}

#endif



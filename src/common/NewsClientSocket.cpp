/*
    Copyright (c) 2017, Ole Stauning
    All rights reserved.

    Use of this source code is governed by a GPLv3-style license that can be found
    in the LICENSE file.
*/

#include "NewsClientSocket.hpp"

#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <algorithm>
#include <list>

namespace ByteFountain
{

//	class RateLimiterTarget

	RateLimiterTarget::RateLimiterTarget(io_service& m_io_service, RateLimiter& limiter):
		m_io_service(m_io_service),
		m_limiter(limiter),
		m_bytesAvailable(0),
		m_bytesMissing(0),
		m_limitEnabled(false),
		m_paused_handler()
	{
		m_limiter.InsertTarget(*this);
	}

	RateLimiterTarget::~RateLimiterTarget()
	{
		m_limiter.RemoveTarget(*this);
	}

	void RateLimiterTarget::AddRXBytes(const std::size_t bytes, std::function<void()> handler)
	{
		m_limiter.AddRXBytes(bytes);
		
		if (m_limitEnabled)
		{
			if (m_bytesAvailable >= bytes)
			{
				m_bytesAvailable -= bytes;
				handler();
			}
			else // bytes > m_bytesAvailable
			{
				m_bytesMissing += bytes - m_bytesAvailable;
				m_bytesAvailable = 0;
				m_paused_handler = handler;
			}
		}
		else
		{
			handler();
		}
	}

	void RateLimiterTarget::AddTXBytes(const std::size_t bytes)
	{
		m_limiter.AddTXBytes(bytes);
	}

	void RateLimiterTarget::Unpause()
	{
		if (m_paused_handler)
		{
			m_io_service.post(m_paused_handler);
			m_paused_handler = nullptr;
		}
	}

	std::size_t RateLimiterTarget::AddTokenBytes(const std::size_t bytes, const std::size_t max)
	{
		if (m_bytesMissing >= bytes)
		{
			m_bytesMissing -= bytes;
		}
		else // m_bytesMissing < bytes
		{
			m_bytesAvailable += bytes - m_bytesMissing;
			m_bytesMissing = 0;
		}
		
		std::size_t spillover = 0;
		if (max>0 && m_bytesAvailable > max)
		{
			spillover = m_bytesAvailable - max;
			m_bytesAvailable = max;
		}
		if (m_paused_handler && m_bytesAvailable>0)
		{
			Unpause();
		}
		return spillover;
	}

	void RateLimiterTarget::SetLimitEnabled(const bool enabled)
	{ 
		m_limitEnabled=enabled; 
		if (!m_limitEnabled) Unpause();
	}

	void RateLimiterTarget::Stop()
	{
		SetLimitEnabled(false);
	}

//	class RateLimiter

	RateLimiter::RateLimiter(io_service& io_service, Logger& logger, 
			std::function<Parameters ()> configure):
		m_io_service(io_service),
		m_logger(logger),
		m_configure(configure),
		m_timer(io_service),
		m_stopping(false),
		m_timer_running(false),
		m_duration(milliseconds(100)),
		m_targets(),
		m_mutex(),
		m_network(),
		Network(m_network)
	{
	}
	
	RateLimiter::~RateLimiter()
	{
		m_timer.cancel();
	}

	void RateLimiter::Stop()
	{
		m_io_service.post([this]()
		{
			m_stopping = true;
			m_timer.cancel();
			for (RateLimiterTarget* target : m_targets)
			{
				target->Stop();
			}
		}
		);
	}
	
	/*private*/ std::size_t RateLimiter::AddTokens(const std::size_t tokens, const std::size_t maxTokens)
	{
		std::size_t tokensPrTarget = tokens / m_targets.size();
		
		if (tokensPrTarget==0) return tokens;

		std::size_t spilloverTokens = tokens % m_targets.size();
		std::list<RateLimiterTarget*> unsaturatedTargets;

		for(RateLimiterTarget* target : m_targets)
		{
			target->SetLimitEnabled(true);

			std::size_t spillover = target->AddTokenBytes(tokensPrTarget, maxTokens);
		
			if (spillover>0) spilloverTokens+=spillover;
			else unsaturatedTargets.push_back(target);
		}
		
		while(spilloverTokens>0 && !unsaturatedTargets.empty())
		{
			tokensPrTarget = spilloverTokens / unsaturatedTargets.size();

			if (tokensPrTarget==0) break;
		
			spilloverTokens = spilloverTokens % unsaturatedTargets.size();
			
			std::list<RateLimiterTarget*> targets;
			targets.swap(unsaturatedTargets);
			
			for(RateLimiterTarget* target : targets)
			{
				std::size_t spillover = target->AddTokenBytes(tokensPrTarget, maxTokens);

				if (spillover>0) spilloverTokens+=spillover;
				else unsaturatedTargets.push_back(target);
			}
		}
		
		return spilloverTokens;
	}
	
	void RateLimiter::AddTokenBytes(const error_code& e)
	{
		if (e != boost::asio::error::operation_aborted && !m_stopping)
		{
			Parameters params = m_configure();

			
			std::unique_lock<std::mutex> lock(m_mutex);
			
//			m_logger << Logger::Error << "Targets=" << m_targets.size() << Logger::End;
			if (m_targets.empty())
			{
				// No clients...
				// Do not continue time-events...
			}
			else
			{
				bool enabled = params.RXBytesPrS!=0 || params.AdditionalTokens!=0;
				
				if (enabled)
				{
					std::size_t tokens = 
						(params.RXBytesPrS * m_duration.total_milliseconds()) / 1000 +
						params.AdditionalTokens;

//					m_logger << Logger::Error << "B/s=" << params.RXBytesPrS << " +B=" << params.AdditionalTokens << " Max=" 
//						<< params.MaxBucketSize << " = " << tokens <<Logger::End;

//					m_logger << Logger::Error << "Tokens=" << tokens << " MaxBucketSize=" << params.MaxBucketSize << Logger::End;

					std::size_t spillover = AddTokens(tokens, tokens * 5);

//					m_logger << Logger::Error << "Tokens=" << tokens << " Spillover=" << spillover << Logger::End;

					if (spillover != tokens)
					{
						SetNextTimerEvent();
						return;
					}
				}
				else
				{
					for (RateLimiterTarget* target : m_targets)
					{
						target->SetLimitEnabled(false);
					}
				}

			}
			
			m_timer_running=false;
		}
	}
	
	void RateLimiter::SetNextTimerEvent()
	{
		m_timer_running=true;
		m_timer.expires_from_now(m_duration);
		m_timer.async_wait(
			[this](const error_code& err)
			{
				if (!err) AddTokenBytes(err);
			}
		);
	}

	
	void RateLimiter::InsertTarget(RateLimiterTarget& ptarget)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_targets.empty()) SetNextTimerEvent();
		m_targets.insert(&ptarget);
		ptarget.SetLimitEnabled(true);
	}
	void RateLimiter::RemoveTarget(RateLimiterTarget& ptarget)
	{ 
		std::unique_lock<std::mutex> lock(m_mutex);
		m_targets.erase(&ptarget); 
	}
	void RateLimiter::AddRXBytes(const std::size_t bytes) 
	{ 
		m_network.RXBytes+=bytes; 
		if (!m_timer_running) SetNextTimerEvent();
	}
	void RateLimiter::AddTXBytes(const std::size_t bytes) { m_network.TXBytes+=bytes; }

}
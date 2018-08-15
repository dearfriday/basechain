//
// Created by boy on 18-6-20.
//
#include<thread>
#include <fc/exception/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp> 
#include <iostream>
#include <wsClient.hpp>
#include <fc/io/json.hpp>
#include "factory.hpp"


#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
using namespace boost::uuids;


#include <vector>
#include <map> 
#include <random> 
using namespace boost;
using namespace std;
int sleeptime = 0;
struct result_body {
	std::string jsonrpc;
	fc::optional<fc::variant> result;
	fc::optional<fc::variant> error;
	int64_t     id;
};

enum option_user
{
	create_user = 0x1,
	transfer_one = create_user + 1,
	transfer_ones = transfer_one + 1,
	publish_comment = transfer_ones + 1,
	comment_vote = publish_comment + 1
 
};
enum task_result
{
	undeal = 0x1,
	sendfail = undeal + 0x1,
	recvfail = sendfail + 0x1,
	fail = recvfail + 1,
	suces= fail+1
};
struct taskresult
{
	uint64_t id;
	option_user op;
	task_result result;
	uint64_t username;
	boost::uuids::uuid permlink;
	uint64_t start_time;
	uint64_t end_time;
	std::string error;
};
enum run_type
{
	balance = 0x1,
	primary = balance + 0x1
};

typedef std::map<int64_t, taskresult> tasklist;
 
FC_REFLECT(result_body, (jsonrpc)(result)(error)(id))
 
using namespace factory;
 

typedef std::vector<std::shared_ptr<std::thread>> thread_pool;
typedef std::map<news::base::account_name, news::base::private_key_type> users;
typedef std::map<news::base::account_name, news::base::private_key_type>::iterator users_it;
class business
{
public:
	business(std::vector<std::string> &ws_, run_type ty= run_type::balance, int thread_count_=8,uint64_t userstartid=1000) :wsaddress(ws_), mode(ty),thread_counts(thread_count_),isstop(false), startid(userstartid)
	{ 
		startid_first = userstartid;
		myusers.insert(std::make_pair(1, NEWS_INIT_PRIVATE_KEY));
		myusers_name.push_back(1);
		wscount = wsaddress.size();
		taskid = 100;
		wsindex= 0; 
	}
 
	std::string getws()
	{ 
		if (mode == run_type::primary|| wscount==1)
			return wsaddress[0];
		if (wsindex = wscount)
		{
			wsindex = 0;
			return wsaddress[0];
		}
		return wsaddress[wsindex];

	}

	void update(uint64_t id, task_result fs= task_result::suces)
	{

		{
			std::lock_guard<std::mutex> lock(mutex_mytask);
			if (mytask[id].id == 0 || mytask[id].op < option_user::create_user)
				return;
			mytask[id].end_time = fc::time_point::now().time_since_epoch().count();
			mytask[id].result = fs;
			{
				if (mytask[id].op == option_user::create_user&&fs== task_result::suces)
				{
					std::lock_guard<std::mutex> lock(mutex_myusers_name);
					myusers_name.push_back(mytask[id].username);
				}
				if (mytask[id].op == option_user::publish_comment && fs == task_result::suces&&permlinkvector.size() < 30)
					permlinkvector.push_back(mytask[id].permlink);

			}
		}
	} 
	void init_money()
	{
		http::client client(getws());
		client.init(
			[](websocketpp::connection_hdl hdl)
		{
			std::cout << "on open " << std::endl;
		},
			[&](websocketpp::connection_hdl hdl, http::message_ptr msg)
		{
			result_body body = fc::json::from_string(msg->get_payload()).as<result_body>();
			if (body.error.valid())
				std::cout << fc::time_point::now().time_since_epoch().to_seconds() << msg->get_payload() << std::endl;

		},
			[](websocketpp::connection_hdl hdl)
		{
			std::cout << "on close " << std::endl;
		},
			[](websocketpp::connection_hdl hdl)
		{
			std::cout << "on fail " << std::endl;

		});
		factory::helper  ff;

		std::cout << "hava " << myusers_name.size() << " account" << std::endl;
		for (int i = 0; i <myusers_name.size(); i++)
		{ 
			if (myusers_name[i] == 1)
				continue;
			uint64_t id = ++taskid;
			auto money = asset(NEWS_SYMBOL, 300 * 10000L);
			auto str=ff.create_transfer(id, myusers[1], 1, myusers_name[i], money);
			string ret = string_json_rpc(id, fc::json::to_string(str));
			client.send_message(ret); 
		}
		client.stop();
		

	}


	bool gen_account()
	{
		
			mytask.clear();
			assert(thread_counts>0); 
			starttime = fc::time_point::now().time_since_epoch().count();
			mytask.clear();
			for (int i = 0; i < thread_counts; i++) 
			{
				std::thread* th = new std::thread([&]() 
				{
					http::client client(getws()); 
					client.init(
						[](websocketpp::connection_hdl hdl)
					{
						std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on open " << std::endl;
					}, 
						[&](websocketpp::connection_hdl hdl, http::message_ptr msg)
					{ 
						result_body body = fc::json::from_string(msg->get_payload()).as<result_body>(); 						
						if (body.error.valid())
						{
							std::cout << fc::time_point::now().time_since_epoch().to_seconds() << msg->get_payload() << std::endl;
							update(body.id, task_result::fail);
						}
						else
						{
							update(body.id);
						}
						 
					}, 
						[](websocketpp::connection_hdl hdl) 
					{
						std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on close " << std::endl;
					}, 
						[](websocketpp::connection_hdl hdl) 
					{
						std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on fail " << std::endl;

					});
					factory::helper  ff;
					uint64_t lastlogtime = 0; 
					while (true)
					{
						if (isstop)
						{
							//boost::this_thread::sleep(boost::posix_time::seconds(60));
							client.stop();
							break;
						} 
						int users_name_size = 0;
						{
							std::lock_guard<std::mutex> lock(mutex_myusers_name);
							users_name_size=myusers_name.size();
						} 
		 
						uint64_t genuser = startid++;						
						news::base::private_key_type genkey; 
					 
						auto str = ff.create_account(NEWS_INIT_PRIVATE_KEY,1,genuser, genkey);
						auto id = taskid++; 
						std::string ret = string_json_rpc(id,fc::json::to_string(str));
						client.send_message(ret); 
						boost::this_thread::sleep(boost::posix_time::microsec(sleeptime));
						taskresult rs;
						auto times= fc::time_point::now().time_since_epoch().count(); 
						if ((times - lastlogtime) > 1000000 * 60 )
						{
							lastlogtime = times;
							std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str()<< "has send " << taskid << " message " << std::endl;
						}
						rs.start_time = times;
						rs.id = id;
						rs.result = task_result::recvfail;
						rs.username = genuser;
						rs.op = option_user::create_user;
						{ 
							std::lock_guard<std::mutex> lock(mutex_mytask);
							mytask.insert(std::make_pair(id, std::move(rs)));
						}						
						{
							std::lock_guard<std::mutex> lock(mutex_); 
							myusers.insert(std::make_pair(genuser, genkey));  
						}


					}   
				});
				std::shared_ptr<std::thread> sps(th);
				th_pool.push_back(sps);
			} 
		return true;
	} 


	bool vote_comment()
	{
		assert(thread_counts>0);
		starttime = fc::time_point::now().time_since_epoch().count();
		mytask.clear();
		for (int i = 0; i < thread_counts; i++)
		{
			std::thread* th = new std::thread([&]()
			{ 
				http::client client(getws());
				client.init(
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on open " << std::endl;
				},
					[&](websocketpp::connection_hdl hdl, http::message_ptr msg)
				{
					result_body body = fc::json::from_string(msg->get_payload()).as<result_body>();
					if (body.error.valid())
					{
						std::cout << fc::time_point::now().time_since_epoch().to_seconds() << msg->get_payload() << std::endl;
						update(body.id, task_result::fail);
					}
					else
					{
						update(body.id); 
					}

				},
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on close " << std::endl;
				},
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on fail " << std::endl;

				});
				factory::helper  ff;
				uint64_t lastlogtime = 0;
				int create_comment = 1;
				std::random_device rd;
				std::default_random_engine engine(rd());
				std::uniform_int_distribution<> dis(-99, 99);
				auto  productor = std::bind(dis, engine);
				while (true)
				{
					if (isstop)
					{
						client.stop();
						break;
					}
					 
						boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
						const string tmp_uuid = boost::uuids::to_string(a_uuid);
						auto id = taskid++;
						signed_transaction str;
						taskresult rs;
						if (create_comment-->0)
						{ 
						   rs.op = option_user::publish_comment;
						   rs.permlink = a_uuid;
						   std::cout << tmp_uuid.c_str() << std::endl;
							str = ff.publish_comment(id, myusers[1], 1, "title", "body test", tmp_uuid.c_str(), "{a:text;}");
							
						}
					else
					{
						if (permlinkvector.size() == 0)
						{
							std::cout << "publish comment fail" << std::endl;
							return;
						}
					
						
						str = ff.comment_vote(id, myusers[1], 1, boost::uuids::to_string(permlinkvector[0]).c_str(), productor());
						rs.op = option_user::comment_vote;
					}
					std::string ret = string_json_rpc(id, fc::json::to_string(str));
					client.send_message(ret); 
					auto times = fc::time_point::now().time_since_epoch().count();
					if ((times - lastlogtime) > 1000000 * 60)
					{
						lastlogtime = times;
						std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "has send " << taskid << " message " << std::endl;
					}
					rs.start_time = times;
					rs.id = id;
					rs.result = task_result::recvfail;
					rs.username = 1; 
					{
						std::lock_guard<std::mutex> lock(mutex_mytask);
						mytask.insert(std::make_pair(id, std::move(rs)));
					}
					if (rs.op == option_user::publish_comment&&create_comment <= 0)
						boost::this_thread::sleep(boost::posix_time::seconds(5));
				}
			});
			std::shared_ptr<std::thread> sps(th);
			th_pool.push_back(sps);
		}
		return true;
	}



	bool publish_comment()
	{
		isstop = false;
		assert(thread_counts>0);
		starttime = fc::time_point::now().time_since_epoch().count();
		mytask.clear();
		for (int i = 0; i < thread_counts; i++)
		{
			std::thread* th = new std::thread([&]()
			{
				http::client client(getws());
				client.init(
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on open " << std::endl;
				},
					[&](websocketpp::connection_hdl hdl, http::message_ptr msg)
				{
					result_body body = fc::json::from_string(msg->get_payload()).as<result_body>();
					if (body.error.valid())
					{
						std::cout << fc::time_point::now().time_since_epoch().to_seconds() << msg->get_payload() << std::endl;
						update(body.id, task_result::fail);
					}
					else
					{
						update(body.id);
					}

				},
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on close " << std::endl;
				},
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on fail " << std::endl;

				});
				factory::helper  ff;
				uint64_t lastlogtime = 0;
				int i = 0;
				while (true)
				{
					if (isstop)
					{
						client.stop();
						break;
					} 
					boost::uuids::uuid a_uuid = boost::uuids::random_generator()();  
					const string tmp_uuid = boost::uuids::to_string(a_uuid);
					auto id = taskid++;
				
					if (++i >= myusers_name.size())
						i = 0; 
					auto users = myusers_name[i];
					auto pk=myusers[users].get_public_key();
					std::cout << pk.to_base58().c_str() << std::endl;
					auto str = ff.publish_comment(id, myusers[users], users, "title","body test",tmp_uuid.c_str(),"{a:text;}"); 
					std::string ret = string_json_rpc(id, fc::json::to_string(str));
					client.send_message(ret);
					taskresult rs;
					auto times = fc::time_point::now().time_since_epoch().count();
					if ((times - lastlogtime) > 1000000 * 60)
					{
						lastlogtime = times;
						std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "has send " << taskid << " message " << std::endl;
					}
					rs.start_time = times;
					rs.id = id;
					rs.result = task_result::recvfail;
					rs.username = 1;
					rs.op = option_user::publish_comment;
					{
						std::lock_guard<std::mutex> lock(mutex_mytask);
						mytask.insert(std::make_pair(id, std::move(rs)));
					} 
				}
			});
			std::shared_ptr<std::thread> sps(th);
			th_pool.push_back(sps);
		}
		return true;
	}


	bool start_business(bool multiple=false)
	{ 
		mytask.clear();
		isstop = false;
		//if (!befor_transfer_one())
		//	return false; 
		//boost::this_thread::sleep(boost::posix_time::seconds(5));
		//std::cout << "gen account ok" << std::endl;
		init_money(); 
		boost::this_thread::sleep(boost::posix_time::seconds(5));
		mytask.clear();
		std::cout << "init money  ok" << std::endl;
		std::cout << "have " << myusers_name.size() << " account to test the transfer" << std::endl;
		if (myusers_name.size() <= 5)
			return false;
		assert(thread_counts>0);
		starttime = fc::time_point::now().time_since_epoch().count();
		mytask.clear();
		for (int i = 0; i < thread_counts; i++) {
			std::thread* th=new std::thread([&]() {
				http::client client(getws());
				client.init(
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on open " << std::endl;
				},
					[&](websocketpp::connection_hdl hdl, http::message_ptr msg)
				{
					
					result_body body = fc::json::from_string(msg->get_payload()).as<result_body>();
					
					if (body.error.valid())
					{
						std::cout << fc::time_point::now().time_since_epoch().to_seconds() << msg->get_payload() << std::endl;
						update(body.id, task_result::fail);
					}
					else
						update(body.id);

				},
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str()<< "on close " << std::endl;
				},
					[](websocketpp::connection_hdl hdl)
				{
					std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str() << "on fail " << std::endl;

				});
				factory::helper  ff;
				static uint64_t lastlogtime=0;
				std::random_device rd;
				std::default_random_engine engine(rd());
				std::uniform_int_distribution<> dis(0, myusers_name.size() - 1);
				auto  productor = std::bind(dis, engine);
				while(true) 
				{
					if (isstop)
					{
						client.stop();
						break;
					}   
					uint64_t from = productor(); 
					uint64_t to = productor();
					if (from == to)
						continue; 
					auto money=asset(NEWS_SYMBOL, (productor()+1)*1L);
					uint64_t id = taskid++;
					signed_transaction str;
					if (multiple)
					{
						uint64_t to1 = productor();
						uint64_t to2 = productor();
						uint64_t to3 = productor();
						std::map<account_name, asset> map;
						map.insert(std::make_pair(to1, money));
						map.insert(std::make_pair(to2, money));
						map.insert(std::make_pair(to3, money));
						str = ff.create_transfers(id, myusers[myusers_name[from]], myusers_name[from], map);
					}
					else
						str = ff.create_transfer(id, myusers[myusers_name[from]],myusers_name[from], myusers_name[to], money);
					std::string ret = string_json_rpc(id,fc::json::to_string(str));
					client.send_message(ret); 
					taskresult rs;
					auto times= fc::time_point::now().time_since_epoch().count();
					if ((times - lastlogtime) > 1000000 * 60 )
					{
						lastlogtime = times;
						std::cout << fc::time_point_sec(fc::time_point::now()).to_iso_string().c_str()<< "has send " << taskid << " message " << std::endl;
					}
					rs.start_time = times;
					rs.id = id;
					rs.result = task_result::recvfail; 
					rs.op = multiple? option_user::transfer_ones: option_user::transfer_one;					 
					{
						std::lock_guard<std::mutex> lock(mutex_mytask);
						mytask.insert(std::make_pair(id, std::move(rs)));
					}
				} 
			});
			std::shared_ptr<std::thread> sps(th);
			th_pool.push_back(sps);
		}
		return true;

	}
	  
	void show_result()
	{
		uint64_t ok = 0;
		uint64_t fail = 0;
		uint64_t responsetime = 0;
		uint64_t responsetime_network_ok = 0;
		uint64_t failnetwork = 0;
		uint64_t long_time = 0;
		uint64_t short_time = 0xFFFFFFFFFFFFFFFF;
		ilog("time:${t}", ("t", (endtime - starttime)/1000000));
		for (auto it : mytask)
		{
			if (it.second.result == task_result::suces)
			{
				assert(it.second.end_time >= it.second.start_time);
				uint64_t current = it.second.end_time - it.second.start_time;
				responsetime += current;
				if (current == 0)
					continue;
				long_time = ((current > long_time) ? current : long_time);
				short_time= ((short_time > current) ? current : short_time);
				ok++;
			}
			else {
				fail++;
				if (it.second.result <= 0x3)			
					failnetwork++;			
				else
					responsetime_network_ok += (it.second.end_time - it.second.start_time);
			}
		}

		ilog("sucess:${t}", ("t",ok));
		ilog("fail:${t}", ("t", fail));
		ilog("fail-network:${t}", ("t", failnetwork));
		ilog("fail-but-network is ok:${t}", ("t", fail-failnetwork));
	    float	rs = ok/((endtime - starttime) / 1000000);
		ilog("ok tps/s:${t}", ("t", rs));
		 rs = (fail - failnetwork) / ((endtime - starttime) / 1000000); 
		ilog("fail but network is ok tps/s:${t} ", ("t", rs));





		ilog("the longest cost:${t}", ("t", long_time));
		ilog("the shortest cost:${t}", ("t", short_time)); 
		ilog("the avg cost:${t}", ("t", responsetime/ ok));

	}
	bool end_business()
	{ 
		isstop = true;
		for (auto it : th_pool)
		{ 
			it->join(); 
		}
		th_pool.clear();
		endtime = fc::time_point::now().time_since_epoch().count();
		return true;
	}
	 
private:
	thread_pool th_pool; 
	int thread_counts;
	bool isstop;
	std::vector<std::string> wsaddress;
	
	std::mutex mutex_myusers_name;
	std::vector<news::base::account_name> myusers_name; 
	users myusers;  
	std::mutex mutex_;


	uint64_t starttime;
	uint64_t endtime;
	tasklist mytask;
	std::mutex mutex_mytask;
	std::atomic<std::uint64_t> taskid;  

	vector<string> endpoint;
	run_type mode;

	uint64_t startid_first;
	std::atomic<std::uint64_t>  startid;

	std::vector<boost::uuids::uuid> permlinkvector;


	int wscount;
	int wsindex; 


	
}; 


int main(int argc, char **argv){
 
		program_options::options_description opts("options");
		opts.add_options()
			("threadpool", program_options::value<std::string>(), "threads count default 8")
			("sleep", program_options::value<std::string>(),"0")
			("startid", program_options::value<std::string>(), "create user startid ,default 1000")
			("ws", program_options::value<std::string>(), "set ws address for example (--ws=\"192.168.0.1:123456-192.168.2.2:7984\")")
			("mode", program_options::value<std::string>(), "send-ws-mode(include primary and balance,default balance)");
		program_options::variables_map vm;
		program_options::store(program_options::parse_command_line(argc, argv, opts), vm);
		std::cout << "for example --ws=\"192.168.2.183:7772\" --mode=balance --threadpool=8 --startid=410000" << std::endl;
		cout << opts << endl;
		run_type type = run_type::balance;
		if (vm.count("mode")) {
			std::string runmode = vm["mode"].as<string>();
			if (runmode.compare("balance") && runmode.compare("primary"))
			{
				std::cout << "error startline of mode";
				return -1;
			}
		} 
		if (vm.count("sleep")) { 
			sleeptime = lexical_cast<int>(vm["sleep"].as<string>().c_str());
		}
		int threadcount = 8;
		if (vm.count("threadpool")) {
			std::string wsaddress = vm["threadpool"].as<string>();
			threadcount = lexical_cast<int>(wsaddress.c_str());
		}
		int startid = 1000;
		if (vm.count("startid")) {
			std::string wsaddress = vm["startid"].as<string>();
			startid = lexical_cast<int>(wsaddress.c_str());
		}

		vector<string> addresses;
		if (vm.count("ws")) {
			std::string wsaddress = vm["ws"].as<string>();
			boost::split(addresses, wsaddress, boost::is_any_of("-"));
		}
		else
		{
			std::cout << "lost startline of ws";
			getchar();
			return -1;
		}
		vector<string> wsaddr;
		for (auto it : addresses)
		{
			std::string str = "ws://";
			str.append(it.c_str());
			wsaddr.push_back(str);
		}
		business bs(wsaddr, type, threadcount,startid);
	__agin:
		
		std::cout << "input: 1: gen account;2:transfer_one;3:transfer_ones:4 :publish_comment 5:vote_comment" << std::endl;

		int i = -1;
		std::cin >> i;
		std::cout << "input x to stop the programe";
		
		switch (i)
		{
		case 1:
			bs.gen_account();
			break;
		case 2:
			bs.start_business();
			break;
		case 3:
			bs.start_business(true);
			break;
		case 4:
			bs.publish_comment();
			break;
		case 5:
			bs.vote_comment();
			break;
			break;
			
		}
		while (true)
		{
			if (getchar() != 'x')
				continue;
			bs.end_business();
				break;

		}
		bs.show_result();

		goto __agin;
		 
		while (true)
		{
			if (getchar() != 'q')
				continue;
			break;
		}
	 
    return 0;
}



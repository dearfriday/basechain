//
// Created by boy on 18-6-26.
//



#include "factory.hpp"
#include <random>

namespace factory{

    signed_transaction& helper::create_account(news::base::private_key_type& sign_pk, news::base::account_name& creator,
                                              news::base::account_name& name,news::base::private_key_type& genprivate) {
        signed_transaction trx;
        create_account_operation cao;
        cao.name = name;
        cao.creator = creator;
		genprivate = fc::ecc::private_key::generate();
        cao.public_key = genprivate.get_public_key();
        trx.set_expiration(fc::time_point_sec(fc::time_point::now().sec_since_epoch() + 300));
        trx.operations.push_back(cao);
        trx.sign(sign_pk, NEWS_CHAIN_ID);
        return trx;
    }

    signed_transaction&
    helper::create_transfer(private_key_type& sign_pk, account_name& from, account_name& to, asset& amount) {
            signed_transaction trx; 
            transfer_operation transfer;
            transfer.from = from;
            transfer.to = to;
			transfer.amount = amount;
            trx.operations.push_back(transfer); 
            trx.set_expiration(fc::time_point_sec(fc::time_point::now().sec_since_epoch() + 300));
            trx.sign(sign_pk, NEWS_CHAIN_ID);  
            return trx;
    }


    std::string&  string_json_rpc(const std::string &str)
	{
		   std::random_device rd;
		   char buff[12] = { 0 };
		   sprintf_s(buff, 12,"%d", rd());  
		   std::string  ret = "{\"jsonrpc\":\"2.0\",\"params\":[\"network_broadcast_api\",\"broadcast_transaction\",{\"trx\":" + str +"}],\"id\":"+buff+",\"method\":\"call\"}";
		  return ret;
    }
}
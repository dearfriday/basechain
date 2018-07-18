#include "vdafactory.hpp"

namespace vdafactory
{
    signed_transaction helper::create_account(news::base::private_key_type sign_pk, news::base::account_name creator,
                                              news::base::account_name name)
    {
        signed_transaction trx;

        create_account_operation cao;
        cao.name = name;
        cao.creator = creator;
        cao.public_key = fc::ecc::private_key::generate().get_public_key();


        trx.set_expiration(fc::time_point_sec(fc::time_point::now().sec_since_epoch() + 10));
        trx.operations.push_back(cao);
        trx.sign(sign_pk, NEWS_CHAIN_ID);
        return trx;
    }

    signed_transaction helper::transfer(news::base::private_key_type sign_pk, news::base::account_name from,
                                              news::base::account_name to, news::base::asset num)
    {
        signed_transaction trx;

        trx.set_expiration(fc::time_point_sec(fc::time_point::now().sec_since_epoch() + 300));

        transfer_operation op;
        op.from = from;
        op.to = to;
        op.amount = num;

        trx.operations.push_back(op);
        trx.sign(sign_pk, NEWS_CHAIN_ID);

        return trx;
    }

    signed_transaction helper::transfers(news::base::private_key_type sign_pk, news::base::account_name from,
                                         std::map<account_name, asset> tos)
    {
        signed_transaction trx;

        trx.set_expiration(fc::time_point_sec(fc::time_point::now().sec_since_epoch() + 300));

        transfers_operation op;
        op.from = from;
        op.to_names = tos;

        trx.operations.push_back(op);
        trx.sign(sign_pk, NEWS_CHAIN_ID);

        return trx;
    }

    std::string  string_json_rpc(const std::string &str)
    {
            std::string ret;
            ret = "{\"jsonrpc\":\"2.0\",\"params\":[\"chain_api\",\"push_transaction\"," + str +"],\"id\":-1,\"method\":\"call\"}";
            return ret;
    }
}
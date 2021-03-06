//
// Created by boy on 18-6-12.
//

#pragma once


//#include <cstdint>
#include <fc/time.hpp>
#include <fc/reflect/reflect.hpp>
#include <news/base/types.hpp>
#include <fc/io/raw.hpp>
#include <news/base/operation.hpp>

namespace news{
    namespace chain{

        using namespace base;

        typedef std::function<public_key_type(const account_name &)> get_key_by_name;


        struct operation_validate_visitor{
            typedef void result_type;
            template <typename  T>
            void operator()(const T &v)const{
                v.validate();
            }
        };

        struct operation_get_sign_name_visitor{
            typedef void result_type;
            operation_get_sign_name_visitor(flat_set<account_name > &_posting, flat_set<account_name > &_owner):post(_posting), owner(_owner){}
            template <typename T>
            void operator()(const T&v)const{
                v.get_sign_name(post);
                v.get_sign_owner(owner);
            }

            template <typename ...T>
            void operator()(const fc::static_variant<T...> v){
                v.visit(*this);
            }
            flat_set<account_name > &post;
            flat_set<account_name > &owner;
        };




        struct transaction{
            uint16_t                ref_block_num = 0;
            uint32_t                ref_block_prefix = 0;
            fc::time_point_sec      expiration;
            vector<operation>       operations;

            transaction_id_type     id() const;
            digest_type             digest() const;
            void                    validate() const;
            digest_type             sig_digest(const chain_id_type &chain_id) const;
            void                    set_expiration(fc::time_point_sec expiration_time);
            void                    set_reference_block(const block_id_type &reference_block);

        };



        struct signed_transaction : public transaction{
            signed_transaction(const transaction &trx = transaction()):transaction(trx){}

            std::vector<signature_type>     signatures;
            const signature_type            &sign(const private_key_type &pk, const chain_id_type &chain_id);
            signature_type                  sign(const private_key_type &pk, const chain_id_type &chain_id)const;

            //TODo get_required_signatures,verify_authority,minimize_required_signatures

            flat_set<public_key_type>       get_signature_keys(const chain_id_type &chain_id) const;
            digest_type                     merkle_digest() const;


            void                            verify_authority(const get_key_by_name &posting, const get_key_by_name &owner, const chain_id_type &chain_id) const;

        };

        struct deatil_signed_transaction : public signed_transaction
        {
            deatil_signed_transaction(){}
            deatil_signed_transaction(const signed_transaction &trx)
            :signed_transaction(trx),trx_id(trx.id()){}

            transaction_id_type          trx_id;
            uint32_t                     block_num = 0;
            uint32_t                     transaction_num = 0;
        };




    }//namespace chain
}//namespace news


namespace fc{
//    void to_variant(const news::chain::signed_transaction &strx, fc::variant &vo);
//    void from_variant(const fc::variant &var, news::chain::signed_transaction &trx);


//    void to_variant(const news::chain::transaction &trx, fc::variant &vo);
//    void from_variant(const fc::variant &var, news::chain::transaction &trx);

}

FC_REFLECT(news::chain::transaction, (ref_block_num)(ref_block_prefix)(expiration)(operations))
FC_REFLECT_DERIVED(news::chain::signed_transaction, (news::chain::transaction), (signatures))
FC_REFLECT_DERIVED(news::chain::deatil_signed_transaction, (news::chain::signed_transaction), (trx_id)(block_num)(transaction_num))
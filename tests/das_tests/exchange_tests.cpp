/*
 * MIT License
 *
 * Copyright (c) 2018 Tech Solutions Malta LTD
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/market_object.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( dascoin_tests, database_fixture )

BOOST_FIXTURE_TEST_SUITE( exchange_unit_tests, database_fixture )

BOOST_AUTO_TEST_CASE( successful_orders_test )
{ try {
    ACTOR(alice);
    ACTOR(bobw);
    VAULT_ACTOR(bob);

    tether_accounts(bobw_id, bob_id);
    issue_webasset("1", alice_id, 100, 100);
    generate_blocks(db.head_block_time() + fc::hours(24) + fc::seconds(1));
    share_type cash, reserved;
    std::tie(cash, reserved) = get_web_asset_amounts(alice_id);

    BOOST_CHECK_EQUAL( cash.value, 100 );
    BOOST_CHECK_EQUAL( reserved.value, 100 );

    adjust_dascoin_reward(500 * DASCOIN_DEFAULT_ASSET_PRECISION);
    adjust_frequency(200);

    set_expiration( db, trx );

    issue_dascoin(bob_id, 100);

    BOOST_CHECK_EQUAL( get_balance(bob_id, get_dascoin_asset_id()), 100 * DASCOIN_DEFAULT_ASSET_PRECISION );

    // Set limit to 100 dascoin
    db.adjust_balance_limit(bob, get_dascoin_asset_id(), 100 * DASCOIN_DEFAULT_ASSET_PRECISION);

    transfer_dascoin_vault_to_wallet(bob_id, bobw_id, 100 * DASCOIN_DEFAULT_ASSET_PRECISION);

    // sell order from cash balance
    create_sell_order(alice_id, asset{100, get_web_asset_id()}, asset{100, get_dascoin_asset_id()});

    // sell order from reserved balance
    create_sell_order(alice_id, asset{0, get_web_asset_id()}, asset{100, get_dascoin_asset_id()}, 100);

    // sell order for dascoin from cash balance
    create_sell_order(bobw_id, asset{1 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}, asset{100, get_web_asset_id()});

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( transfer_custodian )
{ try {
    ACTOR(alice);
    VAULT_ACTOR(alicev);
    CUSTODIAN_ACTOR(bob);
    CUSTODIAN_ACTOR(charlie);
    ACTOR(dave);

    tether_accounts(alice_id, alicev_id);

    // Issue a bunch of assets
    issue_dascoin(alicev_id, 200);
    disable_vault_to_wallet_limit(alicev_id);
    transfer_dascoin_vault_to_wallet(alicev_id, alice_id, 100 * DASCOIN_DEFAULT_ASSET_PRECISION);
    enable_vault_to_wallet_limit(alicev_id);
    issue_webasset("1", alice_id, 100, 100);
    BOOST_CHECK_EQUAL( get_balance(alicev_id, get_dascoin_asset_id()), 100 * DASCOIN_DEFAULT_ASSET_PRECISION );
    BOOST_CHECK_EQUAL( get_balance(alice_id, get_dascoin_asset_id()), 100 * DASCOIN_DEFAULT_ASSET_PRECISION );
    BOOST_CHECK_EQUAL( get_balance(alice_id, get_web_asset_id()), 100 );

    // Expect Success: Transfer from wallet to wallet
    transfer(alice_id, dave_id, asset{100 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()} );
    BOOST_CHECK_EQUAL( get_dascoin_balance(alice_id), 0 );
    BOOST_CHECK_EQUAL( get_dascoin_balance(dave_id), 100 * DASCOIN_DEFAULT_ASSET_PRECISION );

    // Expect Success: Transfer from wallet to custodian
    transfer(dave_id, bob_id, asset{100 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()} );
    BOOST_CHECK_EQUAL( get_dascoin_balance(dave_id), 0 );
    BOOST_CHECK_EQUAL( get_dascoin_balance(bob_id), 100 * DASCOIN_DEFAULT_ASSET_PRECISION );

    // Expect Success: Transfer from custodian to custodian
    transfer(bob_id, charlie_id, asset{50 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()} );
    BOOST_CHECK_EQUAL( get_dascoin_balance(bob_id), 50 * DASCOIN_DEFAULT_ASSET_PRECISION );
    BOOST_CHECK_EQUAL( get_dascoin_balance(charlie_id), 50 * DASCOIN_DEFAULT_ASSET_PRECISION );

    // Expect Success: Transfer from custodian to wallet
    transfer(bob_id, alice_id, asset{50 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()} );
    BOOST_CHECK_EQUAL( get_dascoin_balance(alice_id), 50 * DASCOIN_DEFAULT_ASSET_PRECISION );
    BOOST_CHECK_EQUAL( get_dascoin_balance(bob_id), 0 );

    // Expect Fail: transfer when source or destination is not a wallet or custodian
    GRAPHENE_REQUIRE_THROW( transfer(alicev_id, bob_id, asset{50 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}), fc::exception );
    GRAPHENE_REQUIRE_THROW( transfer(alicev_id, alice_id, asset{50 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}), fc::exception );
    GRAPHENE_REQUIRE_THROW( transfer(alice_id, alicev_id, asset{50 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}), fc::exception );
    GRAPHENE_REQUIRE_THROW( transfer(charlie_id, alicev_id, asset{50 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}), fc::exception );

    // Expect Fail: transfer of asset other than dascoin
    GRAPHENE_REQUIRE_THROW( transfer(alice_id, bob_id, asset{50, get_web_asset_id()}), fc::exception );

    // Expect Fail: transfer from account with insufficient balance
    GRAPHENE_REQUIRE_THROW( transfer(alice_id, bob_id, asset{105 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}), fc::exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( order_not_enough_assets_test )
{ try {
    ACTOR(alice);

    issue_webasset("1", alice_id, 100, 100);
    generate_blocks(db.head_block_time() + fc::hours(24) + fc::seconds(1));
    BOOST_CHECK_EQUAL( get_balance(alice_id, get_web_asset_id()), 100 );
    set_expiration( db, trx );

    // make a huge order from cash balance that ought to fail
    GRAPHENE_REQUIRE_THROW(create_sell_order(alice_id, asset{1000, get_web_asset_id()}, asset{100, get_dascoin_asset_id()}), fc::exception);

    // make a huge order from reserve balance that ought to fail
    GRAPHENE_REQUIRE_THROW(create_sell_order(alice_id, asset{0, get_web_asset_id()}, asset{100, get_dascoin_asset_id()}, 1000), fc::exception);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( cancel_order_test )
{ try {
    ACTOR(alice);

    issue_webasset("1", alice_id, 100, 100);
    generate_blocks(db.head_block_time() + fc::hours(24) + fc::seconds(1));

    share_type cash, reserved;
    std::tie(cash, reserved) = get_web_asset_amounts(alice_id);

    // 100 cash, 100 reserved
    BOOST_CHECK_EQUAL( cash.value, 100 );
    BOOST_CHECK_EQUAL( reserved.value, 100 );

    set_expiration( db, trx );

    // make an order, check balance, then cancel the order
    auto order = create_sell_order(alice_id, asset{100, get_web_asset_id()}, asset{100, get_dascoin_asset_id()});
    BOOST_CHECK_EQUAL( get_balance(alice_id, get_web_asset_id()), 0 );
    cancel_limit_order(*order);
    BOOST_CHECK_EQUAL( get_balance(alice_id, get_web_asset_id()), 100 );

    set_expiration( db, trx );
    order = create_sell_order(alice_id, asset{0, get_web_asset_id()}, asset{100, get_dascoin_asset_id()}, 100);
    std::tie(cash, reserved) = get_web_asset_amounts(alice_id);

    // 100 cash, 0 reserved
    BOOST_CHECK_EQUAL( cash.value, 100 );
    BOOST_CHECK_EQUAL( reserved.value, 0 );

    cancel_limit_order(*order);
    std::tie(cash, reserved) = get_web_asset_amounts(alice_id);

    // 100 cash, 100 reserved
    BOOST_CHECK_EQUAL( cash.value, 100 );
    BOOST_CHECK_EQUAL( reserved.value, 100 );

    set_expiration( db, trx );
    // make an order, let it expire
    do_op(limit_order_create_operation(alice_id, asset{0, get_web_asset_id()}, asset{100, get_dascoin_asset_id()}, 100,
                                       optional<account_id_type>(), db.head_block_time() + fc::seconds(60)));

    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
    generate_block();

    std::tie(cash, reserved) = get_web_asset_amounts(alice_id);
    // 100 cash, 100 reserved
    BOOST_CHECK_EQUAL( cash.value, 100 );
    BOOST_CHECK_EQUAL( reserved.value, 100 );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( exchange_test )
{ try {
    ACTOR(alicew);
    ACTOR(bobw);
    VAULT_ACTOR(bob);
    VAULT_ACTOR(alice);

    const auto check_balances = [this](const account_object& account, share_type expected_cash,
                                       share_type expected_reserved)
    {
        share_type cash, reserved;
        std::tie(cash, reserved) = get_web_asset_amounts(account.id);
        bool amount_ok = (cash == expected_cash && reserved == expected_reserved);
        FC_ASSERT( amount_ok, "On account '${n}': balance = (${c}/${r}), expected = (${ec}/${er})",
                   ("n", account.name)("c", cash)("r", reserved)("ec", expected_cash)("er", expected_reserved));
    };

    // const auto issue_request = [this](const account_object& account, share_type cash, share_type reserved)
    // {
    //     const issue_asset_request_object* p = issue_webasset(account.id, cash, reserved);
    //     FC_ASSERT( p, "Asset request object for '${n}' was not created",("n",account.name));
    //     // ilog( "Creating asset request for '${n}'", ("n", (p->receiver)(db).name));
    //     FC_ASSERT( p->issuer == get_webasset_issuer_id() );
    //     FC_ASSERT( p->receiver == account.id, "Receiver: '${n}', Account: '${an}'", ("n", (p->receiver)(db).name)("an", account.name));
    //     FC_ASSERT( p->amount == cash );
    //     FC_ASSERT( p->asset_id == get_web_asset_id() );
    //     FC_ASSERT( p->reserved_amount == reserved );
    // };

    const auto issue_assets = [&, this](share_type web_assets, share_type web_assets_reserved, share_type expected_web_assets, share_type web_assets_reserved_expected)
    {
        set_expiration( db, trx );
        issue_webasset("1", alice_id, web_assets, web_assets_reserved);
        issue_dascoin(bob_id, 100);

        check_balances(alice, expected_web_assets, web_assets_reserved_expected);
        BOOST_CHECK_EQUAL( get_balance(bob_id, get_dascoin_asset_id()), 100 * DASCOIN_DEFAULT_ASSET_PRECISION );
    };

    issue_assets(1000, 100, 1000, 100);

    tether_accounts(bobw_id, bob_id);
    tether_accounts(alicew_id, alice_id);

    // Set limit to 100 dascoin
    db.adjust_balance_limit(bob, get_dascoin_asset_id(), 100 * DASCOIN_DEFAULT_ASSET_PRECISION);

    transfer_dascoin_vault_to_wallet(bob_id, bobw_id, 100 * DASCOIN_DEFAULT_ASSET_PRECISION);
    transfer_webasset_vault_to_wallet(alice_id, alicew_id, {1000, 100});

    // at this point, alice got 1000+100 web assets and bobw got 100 dascoins

    set_expiration( db, trx );

    // place two orders which will produce a match
    create_sell_order(alicew_id, asset{1 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()},
                      asset{10 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()});
    create_sell_order(bobw_id, asset{10 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()},
                      asset{1 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()});

    // balances: alice 900+100, bob 0
    check_balances(alicew, 900, 100);
    BOOST_CHECK_EQUAL( get_balance(bobw_id, get_dascoin_asset_id()), 90 * DASCOIN_DEFAULT_ASSET_PRECISION );

    const auto &dgpo = get_dynamic_global_properties();
    const auto &dprice = dgpo.last_dascoin_price;
    const price expected_price{ asset{10 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()},
                                asset{1 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()} };
    BOOST_CHECK( dprice == expected_price );

    create_sell_order(alicew_id, asset{2 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()},
                      asset{10 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()});
    create_sell_order(bobw_id, asset{10 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()},
                      asset{2 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()});
    const auto &dgpo2 = get_dynamic_global_properties();
    const auto &dprice2 = dgpo2.last_dascoin_price;
    const price expected_price2{ asset{10 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()},
                                 asset{2 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()}};
    BOOST_CHECK( dprice2 == expected_price2 );

//    issue_assets(1000, 0, 1000, 0);
//    transfer_dascoin_vault_to_wallet(bob_id, bobw_id, 100 * DASCOIN_DEFAULT_ASSET_PRECISION);
//    transfer_webasset_vault_to_wallet(alice_id, alicew_id, {1000, 100});
//
//    set_expiration( db, trx );
//    auto order_id = create_sell_order(alicew_id, asset{100, get_web_asset_id()}, asset{100 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()});
//    create_sell_order(bobw_id, asset{50 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}, asset{50, get_web_asset_id()});
//    cancel_limit_order(*order_id);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( account_to_credit_test )
{ try {
    ACTOR(alice);
    ACTOR(bob);
    VAULT_ACTOR(bobv);
    VAULT_ACTOR(alicev);

    tether_accounts(alice_id, alicev_id);
    tether_accounts(bob_id, bobv_id);

    issue_dascoin(bobv_id, 100);

    // Set limit to 100 dascoin
    db.adjust_balance_limit(bobv, get_dascoin_asset_id(), 100 * DASCOIN_DEFAULT_ASSET_PRECISION);

    transfer_dascoin_vault_to_wallet(bobv_id, bob_id, 100 * DASCOIN_DEFAULT_ASSET_PRECISION);
    issue_webasset("1", alice_id, 100, 100);
    generate_blocks(db.head_block_time() + fc::hours(24) + fc::seconds(1));

    share_type cash, reserved;
    std::tie(cash, reserved) = get_web_asset_amounts(alice_id);

    // 100 cash, 100 reserved
    BOOST_CHECK_EQUAL( cash.value, 100 );
    BOOST_CHECK_EQUAL( reserved.value, 100 );

    set_expiration( db, trx );
    // this will fail - accounts not tethered
    GRAPHENE_REQUIRE_THROW( do_op(limit_order_create_operation(alice_id, asset{0, get_web_asset_id()}, asset{100 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}, 100,
                                   bobv_id, db.head_block_time() + fc::seconds(60))), fc::exception );
    // this will fail - not a vault account
    GRAPHENE_REQUIRE_THROW( do_op(limit_order_create_operation(alice_id, asset{0, get_web_asset_id()}, asset{100 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}, 100,
                                   alice_id, db.head_block_time() + fc::seconds(60))), fc::exception );

    do_op(limit_order_create_operation(alice_id, asset{0, get_web_asset_id()}, asset{100 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}, 100,
                                           alicev_id, db.head_block_time() + fc::seconds(60)));

    create_sell_order(bob_id, asset{100 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()}, asset{100, get_web_asset_id()});

    std::tie(cash, reserved) = get_web_asset_amounts(alice_id);

    BOOST_CHECK_EQUAL( get_balance(alicev_id, get_dascoin_asset_id()), 100 * DASCOIN_DEFAULT_ASSET_PRECISION );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( market_fee_test )
{ try {
    ACTOR(alice);
    ACTOR(bob);
    VAULT_ACTOR(bobv);
    VAULT_ACTOR(alicev);

    tether_accounts(alice_id, alicev_id);
    tether_accounts(bob_id, bobv_id);

    issue_dascoin(bobv_id, 100);

    db.adjust_balance_limit(bobv, get_dascoin_asset_id(), 100 * DASCOIN_DEFAULT_ASSET_PRECISION);

    transfer_dascoin_vault_to_wallet(bobv_id, bob_id, 100 * DASCOIN_DEFAULT_ASSET_PRECISION);
    issue_webasset("1", alice_id, 2 * DASCOIN_FIAT_ASSET_PRECISION, 0);

    // Set market fee of 1 percent on web euro asset:
    const auto& web_asset = db.get_web_asset();
    asset_options opts = web_asset.options;
    opts.market_fee_percent = 100;
    opts.core_exchange_rate.quote.amount = 1;
    opts.core_exchange_rate.quote.asset_id = db.get_web_asset_id();
    asset_update_operation auo;
    auo.issuer = web_asset.issuer;
    auo.asset_to_update = db.get_web_asset_id();
    auo.new_options = opts;
    do_op(auo);

    const auto& wa = db.get_web_asset();
    BOOST_CHECK_EQUAL( wa.options.market_fee_percent, 100);

    // place two orders which will produce a match
    create_sell_order(alice_id, asset{1 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()},
                      asset{1 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()});
    create_sell_order(bob_id, asset{1 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()},
                      asset{1 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()});

    // Since we have a 1% market fee on web euro, we'll get 99 cents:
    BOOST_CHECK_EQUAL( get_balance(bob_id, get_web_asset_id()), 99 );

    // Now set market fee to 0.2%:
    auo.new_options.market_fee_percent = 20;
    do_op(auo);

    const auto& wa2 = db.get_web_asset();
    BOOST_CHECK_EQUAL( wa2.options.market_fee_percent, 20);

    // place two orders which will produce a match
    create_sell_order(alice_id, asset{1 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()},
                      asset{1 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()});
    create_sell_order(bob_id, asset{1 * DASCOIN_DEFAULT_ASSET_PRECISION, get_dascoin_asset_id()},
                      asset{1 * DASCOIN_FIAT_ASSET_PRECISION, get_web_asset_id()});

    // Market fee is now 0.2%, but that is less than 1 euro cent, so market fee is set to 1 cent:
    BOOST_CHECK_EQUAL( get_balance(bob_id, get_web_asset_id()), 198 );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

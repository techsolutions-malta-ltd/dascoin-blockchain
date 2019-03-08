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

#pragma once

#include <graphene/chain/wire_out_with_fee_object.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/protocol/wire_out_with_fee.hpp>

namespace graphene { namespace chain {

  class wire_out_with_fee_evaluator : public evaluator<wire_out_with_fee_evaluator>
  {
  public:
    typedef wire_out_with_fee_operation operation_type;
    void_result do_evaluate(const wire_out_with_fee_operation& op);
    object_id_type do_apply(const wire_out_with_fee_operation& op);

  private:
    const asset_dynamic_data_object* asset_dyn_data_ = nullptr;
    const account_balance_object*    from_balance_obj_ = nullptr;
    const withdrawal_limit_object*   withdrawal_limit_obj_ = nullptr;
    const withdrawal_limit_type*     withdrawal_limit_ = nullptr;
    asset                            spent;
  };

  class wire_out_with_fee_complete_evaluator : public evaluator<wire_out_with_fee_complete_evaluator>
  {
  public:
    typedef wire_out_with_fee_complete_operation operation_type;

    void_result do_evaluate(const wire_out_with_fee_complete_operation& op);
    void_result do_apply(const wire_out_with_fee_complete_operation& op);

  private:
    const wire_out_with_fee_holder_object* holder_ = nullptr;
  };

  class wire_out_with_fee_reject_evaluator : public evaluator<wire_out_with_fee_reject_evaluator>
  {
  public:
    typedef wire_out_with_fee_reject_operation operation_type;

    void_result do_evaluate(const wire_out_with_fee_reject_operation& op);
    void_result do_apply(const wire_out_with_fee_reject_operation& op);

  private:
    const asset_dynamic_data_object* asset_dyn_data_ = nullptr;
    const account_balance_object* balance_obj_ = nullptr;
    const wire_out_with_fee_holder_object* holder_ = nullptr;
  };

} }  // namespace graphene::chain

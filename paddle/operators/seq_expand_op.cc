/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include "paddle/operators/seq_expand_op.h"

namespace paddle {
namespace operators {

using framework::Tensor;

class SeqExpandOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  void InferShape(framework::InferShapeContext* ctx) const override {
    PADDLE_ENFORCE(ctx->HasInput("X"),
                   "Input(X) of SeqExpandOp should not be null.");
    int repeat = ctx->Attrs().Get<int>("repeat");
    framework::DDim out_dim;
    if (repeat == 0) {
      PADDLE_ENFORCE(
          ctx->HasInput("Y"),
          "Input(Y) of SeqExpandOp should not be null while repeat == 0.");
      out_dim = ctx->GetInputDim("Y");
      ctx->ShareLoD("Y", "Out");
    } else {
      out_dim = ctx->GetInputDim("X");
      out_dim[0] = out_dim[0] * repeat;
    }
    PADDLE_ENFORCE(ctx->HasOutput("Out"),
                   "Output(Out) of PadOp should not be null.");
    ctx->SetOutputDim("Out", out_dim);
  }
};

class SeqExpandOpMaker : public framework::OpProtoAndCheckerMaker {
 public:
  SeqExpandOpMaker(framework::OpProto* proto,
                   framework::OpAttrChecker* op_checker)
      : OpProtoAndCheckerMaker(proto, op_checker) {
    // TODO(wanghaoshuang): Add more comments
    AddInput("X", "The input('X') of seq_expand op.");
    AddInput("Y", "The reference input('Y') of seq_expand op.");
    AddOutput("Out", "The output of seq_expand op.");
    AddAttr<int>("repeat", "repeat times").SetDefault(0);
    AddComment(R"DOC(
As an example:

Given:

X = [1, 2 , 3]

and

repeat = 2


then we get

Out.data = [1, 1, 2, 2, 3, 3]
Out.lod = [[0, 2, 4, 6]]

)DOC");
  }
};

class SeqExpandOpGrad : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  void InferShape(framework::InferShapeContext* ctx) const override {
    PADDLE_ENFORCE(ctx->HasInput("X"), "Input(X) should not be null");
    PADDLE_ENFORCE(ctx->HasInput(framework::GradVarName("Out")),
                   "Input(Out@GRAD) should not be null");
    auto x_dims = ctx->GetInputDim("X");
    auto x_grad_name = framework::GradVarName("X");
    if (ctx->HasOutput(x_grad_name)) {
      ctx->SetOutputDim(x_grad_name, x_dims);
    }
  }
};

class SeqExpandOpGradMaker : public framework::SingleGradOpDescMaker {
 public:
  using framework::SingleGradOpDescMaker::SingleGradOpDescMaker;

 protected:
  std::unique_ptr<framework::OpDescBind> Apply() const override {
    auto* bind = new framework::OpDescBind();
    bind->SetInput("X", Input("X"));
    bind->SetInput(framework::GradVarName("Out"), OutputGrad("Out"));
    bind->SetOutput(framework::GradVarName("X"), InputGrad("X"));
    bind->SetAttrMap(Attrs());
    bind->SetType("seq_expand_grad");
    return std::unique_ptr<framework::OpDescBind>(bind);
  }
};

}  // namespace operators
}  // namespace paddle

namespace ops = paddle::operators;

REGISTER_OPERATOR(seq_expand, ops::SeqExpandOp, ops::SeqExpandOpMaker,
                  ops::SeqExpandOpGradMaker);
REGISTER_OPERATOR(seq_expand_grad, ops::SeqExpandOpGrad);
REGISTER_OP_CPU_KERNEL(seq_expand,
                       ops::SeqExpandKernel<paddle::platform::CPUPlace, float>);
REGISTER_OP_CPU_KERNEL(
    seq_expand_grad,
    ops::SeqExpandGradKernel<paddle::platform::CPUPlace, float>);

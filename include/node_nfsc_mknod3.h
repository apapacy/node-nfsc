/*
 * Copyright 2017 Scality
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @authors:
 *    Guillaume Gimenez <ggim@scality.com>
 */
#pragma once
#include <nan.h>
#include "node_nfsc_procedure3.h"


namespace NFS {
    class Client;

    class MkNod3Worker : public Procedure3Worker<MKNOD3args, MKNOD3res> {

        Nan::Utf8String name;
        Nan::Utf8String type;

    public:

        MkNod3Worker(Client *client_,
                     const v8::Local<v8::Value> &parent_fh_,
                     const v8::Local<v8::Value> &name_,
                     const v8::Local<v8::Value> &type,
                     const v8::Local<v8::Value> &mknodData_,
                     Nan::Callback *callback);

    private:

        clnt_stat xdrProc(MKNOD3args *a, MKNOD3res *r, CLIENT *c) NFSC_OVERRIDE {
            return nfsproc3_mknod_3(a, r, c);
        }
        void procSuccess() NFSC_OVERRIDE;
        void procFailure() NFSC_OVERRIDE;
    };
}

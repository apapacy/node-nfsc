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
#include "node_nfsc.h"
#include "node_nfsc_rmdir3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_fattr3.h"
#include "node_nfsc_wcc3.h"

// (dir, name, callback(err, dir_wcc) )
NAN_METHOD(NFS::Client::RmDir3) {
    bool typeError = true;
    if ( info.Length() != 3) {
        Nan::ThrowTypeError("Must be called with 3 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, dir must be a Buffer");
    else if (!info[1]->IsString())
        Nan::ThrowTypeError("Parameter 2, name must be a string");
    else if (!info[2]->IsFunction())
        Nan::ThrowTypeError("Parameter 3, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[2].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::RmDir3Worker(obj, info[0], info[1],
                                                 callback));
}

NFS::RmDir3Worker::RmDir3Worker(NFS::Client *client_,
                                  const v8::Local<v8::Value> &parent_fh_,
                                  const v8::Local<v8::Value> &name_,
                                  Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0),
      parent_fh(),
      name(name_),
      res({})
{
    parent_fh.data.data_val = node::Buffer::Data(parent_fh_);
    parent_fh.data.data_len = node::Buffer::Length(parent_fh_);
}

NFS::RmDir3Worker::~RmDir3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_RMDIR3res, (char *) &res);
}

void NFS::RmDir3Worker::Execute()
{
    if (!client->isMounted()) {
        NFSC_ASPRINTF(&error, NFSC_NOT_MOUNTED);
        return;
    }
    Serialize my(client);
    RMDIR3args args;
    clnt_stat stat;
    args.object.dir = parent_fh;
    args.object.name = *name;
    stat = nfsproc3_rmdir_3(&args, &res, client->getClient());
    if (stat != RPC_SUCCESS) {
        NFSC_ASPRINTF(&error, "%s", rpc_error(stat));
        return;
    }
    if (res.status != NFS3_OK) {
        NFSC_ASPRINTF(&error, "%s", nfs3_error(res.status));
        return;
    }
    success = true;
}

void NFS::RmDir3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
        v8::Local<v8::Value> before, after;
        if (res.RMDIR3res_u.resok.dir_wcc.before.attributes_follow)
            before = node_nfsc_wcc3(res.RMDIR3res_u.resok.dir_wcc.before.pre_op_attr_u.attributes);
        else
            before = Nan::Null();
        if (res.RMDIR3res_u.resok.dir_wcc.after.attributes_follow)
            after = node_nfsc_fattr3(res.RMDIR3res_u.resok.dir_wcc.after.post_op_attr_u.attributes);
        else
            after = Nan::Null();
        wcc->Set(Nan::New("before").ToLocalChecked(), before);
        wcc->Set(Nan::New("after").ToLocalChecked(), after);

        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            wcc,
        };
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
        v8::Local<v8::Value> before, after;
        if (res.RMDIR3res_u.resfail.dir_wcc.before.attributes_follow)
            before = node_nfsc_wcc3(res.RMDIR3res_u.resfail.dir_wcc.before.pre_op_attr_u.attributes);
        else
            before = Nan::Null();
        if (res.RMDIR3res_u.resfail.dir_wcc.after.attributes_follow)
            after = node_nfsc_fattr3(res.RMDIR3res_u.resfail.dir_wcc.after.post_op_attr_u.attributes);
        else
            after = Nan::Null();
        wcc->Set(Nan::New("before").ToLocalChecked(), before);
        wcc->Set(Nan::New("after").ToLocalChecked(), after);
        v8::Local<v8::Value> argv[] = {
            Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked(),
            wcc
        };
        callback->Call(2, argv);
    }
}


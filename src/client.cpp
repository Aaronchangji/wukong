#include "client.h"

client::client(thread_cfg* _cfg, string_server* _str_server): cfg(_cfg)
    , str_server(_str_server), parser(_str_server) {

}

void client::GetId(request_or_reply& req) {
    req.parent_id = cfg->get_inc_id();
}

void client::Send(request_or_reply& req) {
    if (req.parent_id == -1) {
        GetId(req);
    }
    if (req.use_index_vertex() && global_enable_index_partition) {
        int nthread = max(1, min(global_multithread_factor, global_num_server));
        for (int i = 0; i < cfg->nsrvs; i++) {
            for (int j = 0; j < nthread; j++) {
                req.mt_total_thread = nthread;
                req.mt_current_thread = j;
                SendR(cfg, i, cfg->ncwkrs + j, req);
            }
        }
        return ;
    }
    req.first_target = mymath::hash_mod(req.cmd_chains[0], cfg->nsrvs);
    //one-to-one mapping
//    int server_per_client=  cfg->nswkrs  / cfg->ncwkrs;
//    int mid=cfg->ncwkrs + server_per_client*cfg->wid + cfg->get_random() % server_per_client;

    // random
    int tid = cfg->ncwkrs + cfg->get_random() % cfg->nswkrs;
    SendR(cfg, req.first_target, tid, req);
}

request_or_reply client::Recv() {
    request_or_reply r = RecvR(cfg);
    if (r.use_index_vertex() && global_enable_index_partition ) {
        int nthread = max(1, min(global_multithread_factor, global_num_server));
        for (int count = 0; count < cfg->nsrvs * nthread - 1 ; count++) {
            request_or_reply r2 = RecvR(cfg);
            r.silent_row_num += r2.silent_row_num;
            int new_size = r.result_table.size() + r2.result_table.size();
            r.result_table.reserve(new_size);
            r.result_table.insert( r.result_table.end(), r2.result_table.begin(), r2.result_table.end());
        }
    }
    return r;
}

void client::print_result(request_or_reply& reply, int row_to_print) {
    for (int i = 0; i < row_to_print; i++) {
        cout << i + 1 << ":  ";
        for (int c = 0; c < reply.column_num(); c++) {
            int id = reply.get_row_column(i, c);
            if (str_server->id2str.find(id) == str_server->id2str.end()) {
                cout << "NULL  ";
            } else {
                cout << str_server->id2str[reply.get_row_column(i, c)] << "  ";
            }
        }
        cout << endl;
    }
};

'use strict';

//const vConsole = new VConsole();
//const remoteConsole = new RemoteConsole("http://[remote server]/logio-post");
//window.datgui = new dat.GUI();

var vue_options = {
    el: "#top",
    mixins: [mixins_bootstrap],
    store: vue_store,
    router: vue_router,
    data: {
        ipaddress: "",
        ir_list: [],
        params_ir_update: {},
        params_key_update: {},
        params_ir_register: {
            result: {}
        },
    },
    computed: {
        host_url: function(){
            if( this.ipaddress )
                return "http://" + this.ipaddress;
            else
                return ""
        },
    },
    methods: {
        code2disp: function(code){
            var item = keyid_list.find(item => item.keyid == code);
            if( item )
                return item.disp;
            else
                return null;
        },
        mod2list: function(mod){
            var list = [];
            const mod_list = ["LEFT_CTRL", "LEFT_SHIFT", "LEFT_ALT", "LEFT_GUI", "RIGHT_CTRL", "RIGHT_SHIFT", "RIGHT_ALT", "RIGHT_GUI"];
            for( var i = 0 ; i < mod_list.length ; i++ )
                if( mod & (0x01 << i) )
                    list.push(mod_list[i]);
            return list;
        },

        ir_reboot: async function(){
            var params = {
                endpoint: "/irhid-reboot",
                params: {}
            };
            var result = await do_post(this.host_url + "/endpoint", params);
            console.log(result);
            if( result.status != 'OK' )
                throw "result.status not OK";
        },
        irlist_refresh: async function(){
            var params = {
                endpoint: "/irhid-ir-list",
                params: {}
            };
            var result = await do_post(this.host_url + "/endpoint", params);
            console.log(result);
            if( result.status != 'OK' )
                throw "result.status not OK";

            this.ir_list = result.result.ir_list;
        },
        start_ir_update: async function(index){
            this.params_ir_update = {
                value: this.ir_list[index].value,
                name: this.ir_list[index].name
            };
            this.dialog_open("#ir_update_dialog");
        },
        call_ir_update: async function(){
            var params = {
                endpoint: "/irhid-ir-update",
                params: {
                    value: this.params_ir_update.value,
                    name: this.params_ir_update.name,
                }
            }
            var result = await do_post(this.host_url + "/endpoint", params);
            console.log(result);
            if( result.status != 'OK' )
                throw "result.status not OK";

            this.dialog_close("#ir_update_dialog");
            this.irlist_refresh();
        },
        start_ir_delete: async function(index){
            if( !confirm("本当に削除しますか？") )
                return;

            var params = {
                endpoint: "/irhid-ir-delete",
                params: {
                    value: this.ir_list[index].value
                }
            }
            var result = await do_post(this.host_url + "/endpoint", params);
            console.log(result);
            if( result.status != 'OK' )
                throw "result.status not OK";

            this.irlist_refresh();
        },
        start_key_update: async function(index){
            this.params_key_update = {
                value: this.ir_list[index].value,
                key_type: this.ir_list[index].key_type
            };
            if( this.ir_list[index].key_type == 'text' ){
                this.params_key_update.text = this.ir_list[index].text;
            }else if( this.ir_list[index].key_type == 'key' ){
                this.params_key_update.mods = [];
                var mod = this.ir_list[index].mod;
                if( mod & (0x01 << 0) ) this.params_key_update.mods.push("LEFT_CTRL");
                if( mod & (0x01 << 1) ) this.params_key_update.mods.push("LEFT_SHIFT");
                if( mod & (0x01 << 2) ) this.params_key_update.mods.push("LEFT_ALT");
                if( mod & (0x01 << 3) ) this.params_key_update.mods.push("LEFT_GUI");
                if( mod & (0x01 << 4) ) this.params_key_update.mods.push("RIGHT_CTRL");
                if( mod & (0x01 << 5) ) this.params_key_update.mods.push("RIGHT_SHIFT");
                if( mod & (0x01 << 6) ) this.params_key_update.mods.push("RIGHT_ALT");
                if( mod & (0x01 << 7) ) this.params_key_update.mods.push("RIGHT_GUI");
                this.params_key_update.code = this.ir_list[index].code;
            }
            this.dialog_open("#key_update_dialog");
        },
        call_key_update: async function(){
            var params = {
                endpoint: "/irhid-key-update",
                params: {
                    value: this.params_key_update.value,
                    key_type: this.params_key_update.key_type,
                }
            };
            if( this.params_key_update.key_type == 'text' ){
                params.params.text = this.params_key_update.text;
            }else if( this.params_key_update.key_type == 'key' ){
                params.params.mod = 0x00;
                if( this.params_key_update.mods ){
                    if( this.params_key_update.mods.indexOf("LEFT_CTRL") >= 0 ) params.params.mod |= (0x01 << 0);
                    if( this.params_key_update.mods.indexOf("LEFT_SHIFT") >= 0 ) params.params.mod |= (0x01 << 1);
                    if( this.params_key_update.mods.indexOf("LEFT_ALT") >= 0 ) params.params.mod |= (0x01 << 2);
                    if( this.params_key_update.mods.indexOf("LEFT_GUI") >= 0 ) params.params.mod |= (0x01 << 3);
                    if( this.params_key_update.mods.indexOf("RIGHT_CTRL") >= 0 ) params.params.mod |= (0x01 << 4);
                    if( this.params_key_update.mods.indexOf("RIGHT_SHIFT") >= 0 ) params.params.mod |= (0x01 << 5);
                    if( this.params_key_update.mods.indexOf("RIGHT_ALT") >= 0 ) params.params.mod |= (0x01 << 6);
                    if( this.params_key_update.mods.indexOf("RIGHT_GUI") >= 0 ) params.params.mod |= (0x01 << 7);
                }
                params.params.code = this.params_key_update.code;
            }
            var result = await do_post(this.host_url + "/endpoint", params);
            console.log(result);
            if( result.status != 'OK' )
                throw "result.status not OK";

            this.dialog_close("#key_update_dialog");
            this.irlist_refresh();
        },
        ir_register: async function(){
            try{
                this.progress_open("リモコンのボタンを押してください。");
                var params = {
                    endpoint: "/irhid-ir-check",
                    params: {}
                };
                var result = await do_post(this.host_url + "/endpoint", params);
                console.log(result);
                if( result.status != 'OK' )
                    throw "result.status not OK";

                var result;
                do{
                    var params = {
                        endpoint: "/irhid-ir-status",
                        params: {}
                    };
                    result = await do_post(this.host_url + "/endpoint", params);
                    console.log(result);
                    if( result.status != 'OK' )
                        throw "result.status not OK";                    
                    if( !result.result.irReceiving )
                        break;

                    await wait_msec(1000);
                }while(true);
                if( !result.result.irReceived )
                    return;

                this.params_ir_register.result = result.result;
                this.dialog_open('#ir_register_dialog');
            }finally{
                this.progress_close();
            }
        },
        call_ir_register: async function(){
            var params = {
                endpoint: "/irhid-ir-register",
                params: {
                    name: this.params_ir_register.name,
                    value: this.params_ir_register.result.value
                }
            };
            var result = await do_post(this.host_url + "/endpoint", params);
            console.log(result);

            this.dialog_close("#ir_register_dialog");
            this.irlist_refresh();
        },
    },
    created: function(){
    },
    mounted: function(){
        proc_load();
    }
};
vue_add_data(vue_options, { progress_title: '' }); // for progress-dialog
vue_add_global_components(components_bootstrap);
vue_add_global_components(components_utils);

/* add additional components */
  
window.vue = new Vue( vue_options );

async function wait_msec(msec){
    return new Promise(resolve => setTimeout(resolve, msec) );
}

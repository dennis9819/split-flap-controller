/*
* SplitFlapController Web GUI JavaScript
* Copyright (c) 2025 - 2026 Dennis Gunia (www.dennisgunia.de)
* Licensed under the AGPL-3.0 license.
*/

// global variables
let recall_buffer = [];    // recall history buffer
let favorites_buffer = []; // favorites buffer
let sfc = new SFC();       // SFC WebSocket connection object
let modules = [];          // module list

// After Document load, change view according to url and connect to ws server
document.addEventListener("DOMContentLoaded", function () {
    const urlSection = location.href.split("#")[1];
    if (urlSection) {
        change_view(urlSection);
    }
    sfc.connect();
    load_favorites();
    render_str_len();
});

// cahnge view function / change tabs
function change_view(id) {
    document.getElementById('view_display').style.display = 'none';
    document.getElementById('view_conf_modules').style.display = 'none';
    document.getElementById('view_storage').style.display = 'none';
    document.getElementById(id).style.display = 'block';
    switch (id) {
        case 'view_display':
            break;
        case 'view_conf_modules':
            load_module_conf();
            break;
    }
    // append to url
    //location.href = "test"
    const newHref = `${location.href.split("#")[0]}#${id}`;
    location.href = newHref;

    // close menu
    document.getElementById('menu_dropdown').removeAttribute('open');
}

// load favorites from local storage
function load_favorites() {
    const local_storage_data = localStorage.getItem("favorites");
    try {
        favorites_buffer = JSON.parse(local_storage_data);
        if (favorites_buffer === null || !Array.isArray(favorites_buffer)) {
            favorites_buffer = [];
            console.log("no favorites found. Initialize empty list.");
        }
        push_recall_history();

    } catch (error) {
        localStorage.removeItem("favorites");
        favorites_buffer = [];
        console.error("cannot load favorites", error)
    }
}


/* utility function to format int to hex string */
function toAddressStr(address) {
    const hexbase = address.toString(16).padStart(4, '0').toUpperCase();
    return `0x${hexbase}`;
}

// notification function
let messageCounter = 0;
function notify(severity, text) {
    const template = document.querySelector("#notification");
    const clone = template.content.cloneNode(true);
    const tbody = document.querySelector("#message_container");

    clone.querySelector("div").classList.add(`notification-${severity}`)
    clone.querySelector("div").querySelector("p").innerHTML = text;
    const messageDivID = `notification-dyn-${messageCounter}`
    clone.querySelector("div").id = messageDivID;
    messageCounter += 1;

    setTimeout(function () {
        //document.querySelector(`#${messageDivID}`).style.transition = "opacity 1s ease";
        document.querySelector(`#${messageDivID}`).style.opacity = 0;
        setTimeout(function () {
            document.querySelector(`#${messageDivID}`).remove();
        }, 1001);
    }, 2000);

    tbody.appendChild(clone);
    document.querySelector(`#${messageDivID}`).style.opacity = 0.7;

}

function push_recall_history(data) {
    if (data) {
        recall_buffer.push(data);
    }
    localStorage.setItem("favorites", JSON.stringify(favorites_buffer));
    render_recall_table();
}

function btn_display() {
    const text = document.getElementById("form_display_str").value;
    const x = Number(document.getElementById("form_display_x").value);
    const y = Number(document.getElementById("form_display_y").value);
    const mode = document.getElementById("form_display_mode").value;
    console.log(mode)
    const msg = {
        "command": "dm_print",
        "string": text,
        "x": x,
        "y": y,
        "mode": Number(mode)
    }
    sfc.command_async(msg);

    push_recall_history({
        "string": text,
        "x": x,
        "y": y,
        "full": Number(mode)
    })
}

function btn_clear() {
    const msg = {
        "command": "dm_clear",
    }
    sfc.command_async(msg);
}

function btn_reset_module(address) {
    const msg = {
        "command": "dr_reset",
        "address": address
    }
    sfc.command_callback(msg, () => {
        notify("success", "Module reset!");
        load_module_conf();
    });
}

function btn_save() {
    const msg = {
        "command": "dm_save",
    }
    sfc.command_async(msg);
}

function btn_load() {
    const msg = {
        "command": "dm_load",
    }
    sfc.command_async(msg);
}


function load_module_conf() {
    if (sfc.socket) {
        document.getElementById('btn_refresh').ariaBusy = 'true';
        document.getElementById('btn_refresh').disabled = true;
        // load config
        console.log("loading module config...");
        sfc.command_callback({ "command": "dm_dump" }, (data) => {
            if (data["devices"]) {
                modules = data["devices"];
                // generate table
                render_module_table();
                document.getElementById('btn_refresh').ariaBusy = 'false';
                document.getElementById('btn_refresh').disabled = false;
            }
            console.log("module config loaded.");
        });
    }
}

function render_str_len() {
    const str_input = document.getElementById("form_display_str");
    const str_len_display = document.getElementById("form_display_str_len");
    str_len_display.textContent = `Length: ${str_input.value.length} chars`;
}

function render_module_table() {
    // clear table
    const node_module_table = document.querySelector("#module_list");
    node_module_table.innerHTML = "";

    // loop over all modules
    for (let i = 0; i < modules.length; i++) {
        const mod = modules[i];
        const template = document.querySelector("#module_list_template"); // get template
        const node_table_row_current = template.content.cloneNode(true);
        // set header text
        let node_text_header = node_table_row_current.querySelector("summary");
        node_text_header.textContent = `Mod ${mod["id"]} : ${mod["status"]["device"]} | Addr: ${toAddressStr(mod["address"])}, Pos: (${mod["position"]["x"]}, ${mod["position"]["y"]})`;
        switch (mod["status"]["device"]) {
            case 'ONLINE':
                node_text_header.classList.add("pico-color-jade-500");
                break
            case 'OFFLINE':
                node_text_header.classList.add("pico-color-red-500");
                break;
        }

        // fill table
        let nodes_table_columns = node_table_row_current.querySelectorAll("td");
        nodes_table_columns[0].textContent = mod["id"];
        nodes_table_columns[2].textContent = `${mod["address"]} (${toAddressStr(mod["address"])})`;
        nodes_table_columns[4].textContent = `${mod["calibration"]} (${toAddressStr(mod["calibration"])})`;
        nodes_table_columns[6].textContent = mod["status"]["device"];
        nodes_table_columns[8].textContent = mod["status"]["rotations"];
        nodes_table_columns[10].textContent = `${mod["position"]["x"]}, ${mod["position"]["y"]}`;
        nodes_table_columns[12].textContent = mod["status"]["power"];
        nodes_table_columns[14].textContent = `${Math.round((mod["status"]["voltage"]) * 100) / 100} V`;
        nodes_table_columns[16].textContent = `id: ${mod["flapID"]}, char: '${mod["flapChar"]}'`;
        nodes_table_columns[20].textContent = mod["firmwareVersion"];
        nodes_table_columns[13].querySelector("input").checked = mod["status"]["power"];

        // prepare flags
        let flags = [];
        Object.keys(mod["status"]["flags"]).forEach(flag => {
            if (mod["status"]["flags"][flag]) {
                flags.push(flag);
            }
        });
        if (flags.length > 0) {
            nodes_table_columns[18].textContent = flags.join(", ");
        }

        // define button actions
        node_table_row_current.querySelector(".btn_reset").onclick = function () {
            // reset module button
            btn_reset_module(mod["address"]);
        };
        node_table_row_current.querySelector(".btn_remove").onclick = function () {
            // remove module button
            const msg = {
                "command": "dm_remove",
                "id": mod["id"],
            }
            sfc.command_callback(msg, () => {
                notify("success", "Module removed!");
                load_module_conf();
            });

        };
        nodes_table_columns[5].querySelector("button").onclick = function () {
            // calibration change button
            const dialog = document.getElementById("dialog_change_calibration");
            dialog.showModal();
            const dialog_el = dialog.querySelector("article").querySelector("section").querySelector("footer");

            dialog_el.querySelector(".btn_confirm").onclick = function () {
                const msg = {
                    "command": "dr_setcalibration",
                    "address": mod["address"],
                    "calibration": Number(dialog.querySelector("#form_change_calibration_data").value)
                }
                sfc.command_callback(msg, () => {
                    notify("success", "Calibration changed!");
                    sfc.command_callback({ "command": "dr_reset", "address": mod["address"] }, () => {
                        load_module_conf();
                    });
                });
                dialog.close();
            };
            dialog_el.querySelector(".btn_cancel").onclick = function () {
                dialog.close();
            }
        };
        nodes_table_columns[13].querySelector("input").onclick = function () {
            // power button
            const msg = {
                "command": "dr_power",
                "address": mod["address"],
                "power": nodes_table_columns[13].querySelector("input").checked
            }
            sfc.command_callback(msg, () => {
                notify("success", "Power state changed!");
                load_module_conf();
            });
        }


        node_module_table.appendChild(node_table_row_current);
    }

}

function render_recall_table() {
    const node_recall_table_content = document.querySelector("#recall_list");
    node_recall_table_content.innerHTML = "";

    const template = document.querySelector("#recall_list_template");
    let recall_list = recall_buffer.concat(favorites_buffer); // merge both lists, favorites first (because list is rendered reversed)

    // loop over all recall items and add to html node
    for (let i = recall_list.length - 1; i >= 0; i--) {
        const item = recall_list[i];
        const node_recall_table_row = template.content.cloneNode(true);
        let nodes_recall_table_columns = node_recall_table_row.querySelectorAll("td");
        // fill table rows
        nodes_recall_table_columns[0].querySelector('.recall_text').textContent = item["string"];
        nodes_recall_table_columns[1].textContent = `${item["x"]}, ${item["y"]}`;
        switch (item["full"]) {
            case 1:
                nodes_recall_table_columns[2].textContent = "Direct";
                break;
            case 0:
                nodes_recall_table_columns[2].textContent = "Full Rotation";
                break;
            case 2:
                nodes_recall_table_columns[2].textContent = "Sync";
                break;
        }

        // define button actions
        node_recall_table_row.querySelector(".btn_recall_load").onclick = function () {
            document.getElementById("form_display_str").value = item["string"];
            document.getElementById("form_display_x").value = item["x"];
            document.getElementById("form_display_y").value = item["y"];
            document.getElementById("form_display_mode").value = item["full"];
            render_str_len();
        };
        node_recall_table_row.querySelector(".btn_recall_display").onclick = function () {
            document.getElementById("form_display_str").value = item["string"];
            document.getElementById("form_display_x").value = item["x"];
            document.getElementById("form_display_y").value = item["y"];
            document.getElementById("form_display_mode").value = item["full"];
            render_str_len();
            btn_display();
        };
        let button_recall_add_fav = node_recall_table_row.querySelector(".btn_recall_fav")
        let button_recall_remove_fav = node_recall_table_row.querySelector(".btn_recall_fav_remove")
        if (item.isfav) {
            button_recall_add_fav.parentNode.removeChild(button_recall_add_fav); // remove add to fav button
            button_recall_remove_fav.onclick = function () {
                // remove from favorites
                let index = favorites_buffer.findIndex(favitem =>
                    favitem["string"] === item["string"] &&
                    favitem["x"] === item["x"] &&
                    favitem["y"] === item["y"] &&
                    favitem["full"] === item["full"]
                );
                if (index !== -1) {
                    favorites_buffer.splice(index, 1);
                    notify("success", "Removed to favorites!");
                    localStorage.setItem("favorites", JSON.stringify(favorites_buffer));
                }
                render_recall_table();
            }
        } else {
            nodes_recall_table_columns[0].removeChild(nodes_recall_table_columns[0].querySelector('.bi')); // remove fav icon
            button_recall_remove_fav.parentNode.removeChild(button_recall_remove_fav); // remove remove from fav button
            button_recall_add_fav.onclick = function () {
                let new_favittem = JSON.parse(JSON.stringify(item));
                new_favittem.isfav = true;
                favorites_buffer.push(new_favittem);
                notify("success", "Added to favorites!");
                localStorage.setItem("favorites", JSON.stringify(favorites_buffer));
                render_recall_table();
            }
        }
        node_recall_table_content.appendChild(node_recall_table_row);
    }
}

function display_dialog_change_address() {
    const dialog = document.getElementById("change_address");
    dialog.showModal();
    const dialog_el = dialog.querySelector("article").querySelector("section").querySelector("footer");


    dialog_el.querySelector(".btn_confirm").onclick = function () {
        const addr_old = Number(dialog.querySelector("#form_change_address_old").value);
        const addr_new = Number(dialog.querySelector("#form_change_address_new").value);
        const msg = {
            "command": "dr_setaddress",
            "address": addr_old,
            "newaddress": addr_new,
        }
        sfc.command_async(msg);
        setTimeout(function () {
            btn_reset_module(addr_old);
            load_module_conf();
        }, 200);
        dialog.close();
    };
    dialog_el.querySelector(".btn_cancel").onclick = function () {
        dialog.close();
    }
}

function display_dialog_add_device() {
    const dialog = document.getElementById("dialog_add_device");
    dialog.showModal();
    const dialog_el = dialog.querySelector("article").querySelector("section").querySelector("footer");

    dialog_el.querySelector(".btn_confirm").onclick = function () {
        const msg = {
            "command": "dm_register",
            "address": Number(dialog.querySelector("#form_add_device_addr").value),
            "x": Number(dialog.querySelector("#form_add_device_x").value),
            "y": Number(dialog.querySelector("#form_add_device_y").value)
        }
        sfc.command_async(msg);
        setTimeout(function () {
            load_module_conf();
        }, 200);
        dialog.close();
    };
    dialog_el.querySelector(".btn_cancel").onclick = function () {
        dialog.close();
    }
}


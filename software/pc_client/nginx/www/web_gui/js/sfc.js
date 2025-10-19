// Create WebSocket connection.
let hostname = location.host;
if (hostname === "") {
  hostname = "localhost";
}
console.log(`Connecting to ws://${hostname}`);

const socket = new WebSocket(`ws://${hostname}/manage/`);

// Connection opened
socket.addEventListener("open", (event) => {
  SFCCommandAsync({ "command": "dm_load" });

  //changeView("view_storage");
});

socket.addEventListener("message", (event) => {
  switch (last_command) {
    case "dm_dump":
      parse_module_conf(JSON.parse(event.data));
      break;
  }
});

function toAddressStr(address) {
  const hexbase = address.toString(16).padStart(4, '0').toUpperCase();
  return `0x${hexbase}`;
}

// Send command
function SFCCommandAsync(data) {
  last_command = data.command;
  socket.send(JSON.stringify(data));
}

function btn_display() {
  const text = document.getElementById("form_display_str").value;
  const x = Number(document.getElementById("form_display_x").value);
  const y = Number(document.getElementById("form_display_y").value);

  const msg = {
    "command": "dm_print",
    "string": text,
    "x": x,
    "y": y
  }
  SFCCommandAsync(msg);
}

function btn_clear() {
  const msg = {
    "command": "dm_clear",
  }
  SFCCommandAsync(msg);
}

function btn_reset_module(address) {
  const msg = {
    "command": "dr_reset",
    "address": address
  }
  SFCCommandAsync(msg);
}

function changeView(id) {
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
}

let modules = [];

function load_module_conf() {
  document.getElementById('btn_refresh').ariaBusy = 'true';
  document.getElementById('btn_refresh').disabled = true;

  SFCCommandAsync({ "command": "dm_dump" });

}

function parse_module_conf(data) {
  if (data["devices"]) {
    modules = data["devices"];

    const tbody = document.querySelector("#module_list");
    tbody.innerHTML = "";

    for (let i = 0; i < modules.length; i++) {
      const mod = modules[i];
      const template = document.querySelector("#module_list_template");

      const clone = template.content.cloneNode(true);
      let summary = clone.querySelector("summary");
      summary.textContent = `Module ID ${mod["id"]} : ${mod["status"]["device"]}`;
      switch (mod["status"]["device"]) {
        case 'ONLINE':
          summary.classList.add("pico-color-jade-500");
          break
        case 'OFFLINE':
          summary.classList.add("pico-color-red-500");
          break;
      }
      // fill table
      let td = clone.querySelectorAll("td");
      td[0].textContent = mod["id"];
      td[2].textContent = `${mod["address"]} (${toAddressStr(mod["address"])})`;
      td[4].textContent = `${mod["calibration"]} (${toAddressStr(mod["calibration"])})`;
      td[6].textContent = mod["status"]["device"];
      td[8].textContent = mod["status"]["rotations"];
      td[10].textContent = `${mod["position"]["x"]}, ${mod["position"]["y"]}`;
      td[12].textContent = mod["status"]["power"];
      td[14].textContent = `${Math.round((mod["status"]["voltage"]) * 100) / 100} V`;
      td[16].textContent = `id: ${mod["flapID"]}, char: '${mod["flapChar"]}'`;

      // prepare flags
      let flags = [];
      Object.keys(mod["status"]["flags"]).forEach(flag => {
        if (mod["status"]["flags"][flag]) {
          flags.push(flag);
        }
      });
      if (flags.length > 0) {
        td[18].textContent = flags.join(", ");
      }
      clone.querySelector(".btn_reset").onclick = function () {
        btn_reset_module(mod["address"]);
      };
      clone.querySelector(".btn_remove").onclick = function () {
        const msg = {
          "command": "dm_remove",
          "id": mod["id"],
        }
        SFCCommandAsync(msg);
        setTimeout(function () {
          load_module_conf();
        }, 200);
      };
      td[13].querySelector("input").checked = mod["status"]["power"];

      // define set calibration button
      td[5].querySelector("button").onclick = function () {
        const dialog = document.getElementById("dialog_change_calibration");
        dialog.showModal();
        const dialog_el = dialog.querySelector("article").querySelector("section").querySelector("footer");

        dialog_el.querySelector(".btn_confirm").onclick = function () {
          const msg = {
            "command": "dr_setcalibration",
            "address": mod["address"],
            "calibration": Number(dialog.querySelector("#form_change_calibration_data").value)
          }
          SFCCommandAsync(msg);
          setTimeout(function () {
            btn_reset_module(mod["address"]);
            load_module_conf();
          }, 200);
          dialog.close();
        };
        dialog_el.querySelector(".btn_cancel").onclick = function () {
          dialog.close();
        }
      };
      // define power button
      td[13].querySelector("input").onclick = function () {
        const msg = {
          "command": "dr_power",
          "address": mod["address"],
          "power": td[13].querySelector("input").checked
        }
        SFCCommandAsync(msg);
        setTimeout(function () {
          load_module_conf();
        }, 200);
      }


      tbody.appendChild(clone);
      document.getElementById('btn_refresh').ariaBusy = 'false';
      document.getElementById('btn_refresh').disabled = false;

    }

  }
  console.log(modules);
}

function display_dialog_change_address() {
  const dialog = document.getElementById("change_address");
  dialog.showModal();
  const dialog_el = dialog.querySelector("article").querySelector("section").querySelector("footer");
  const addr_old = Number(dialog.querySelector("#form_change_address_old").value);
  const addr_new = Number(dialog.querySelector("#form_change_address_new").value);

  dialog_el.querySelector(".btn_confirm").onclick = function () {
    const msg = {
      "command": "dr_setaddress",
      "address": addr_old,
      "newaddress": addr_new,
    }
    SFCCommandAsync(msg);
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
    SFCCommandAsync(msg);
    setTimeout(function () {
      load_module_conf();
    }, 200);
    dialog.close();
  };
  dialog_el.querySelector(".btn_cancel").onclick = function () {
    dialog.close();
  }
}

function btn_save(){
  const msg = {
    "command": "dm_save",
  }
  SFCCommandAsync(msg);
}

function btn_load(){
  const msg = {
    "command": "dm_load",
  }
  SFCCommandAsync(msg);
}
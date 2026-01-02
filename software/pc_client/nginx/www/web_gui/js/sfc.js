/*
* SplitFlapController Web GUI JavaScript
* Copyright (c) 2025 - 2026 Dennis Gunia (www.dennisgunia.de)
* Licensed under the AGPL-3.0 license.
*/

class SFC {
  constructor() {
    this.socket = null;
    this.callback_func = null;
  }

  connect() {
    // Create WebSocket connection.
    let hostname = location.host;
    if (hostname === "") {
      hostname = "localhost";

    }
    console.log(`Connecting to ws://${hostname}`);

    this.socket = new WebSocket(`ws://${hostname}/manage/`);
    // Connection opened
    this.socket.addEventListener("open", (event) => {
      notify("success", "connected!");
      load_module_conf();

    });
    this.socket.addEventListener("close", (event) => {
      notify("warn", "ws connection failed!. Reconnect in 5s");
      setTimeout(() => {
        this.connect();
      }, 5000)
    });
    this.socket.addEventListener("message", (event) => {
      const data = JSON.parse(event.data)
      console.log("Message from server ", data, this.callback_func);
      if (this.callback_func) {
        const callback_func = this.callback_func;
        this.callback_func = null;
        callback_func(data);
        
        return;
      }else if (data.ack) {
        notify("success", "command sent!");
      }
    });
  }

  command_async(data) {
    if (this.socket && this.socket.readyState !== WebSocket.CLOSED) {
      this.last_command = data.command;
      this.socket.send(JSON.stringify(data));
    } else {
      notify("error", "ws not connected!");
    }
  }

  command_callback(data, callback) {
    if (this.socket && this.socket.readyState !== WebSocket.CLOSED) {
      this.last_command = data.command;
      this.socket.send(JSON.stringify(data));
      this.callback_func = callback;
    } else {
      notify("error", "ws not connected!");
    }
  }
}

const host = window.location.hostname;
const port = window.location.port;
let apiUrl = `http://${host}:${port}/api/`;

const passwordCookie = getCookie("rfu-password");
if (!passwordCookie) {
  window.location.href = "index.html";
}
authenticatePassword(passwordCookie);

document.getElementById("ap_mode").checked = true;

/* TODO - Implement the API Endpoint for this
// Get the current settings from the API
fetch(apiUrl + 'conf')
    .then(function (response) {
        return response.json();
    })
    .then(function (data) {
        document.getElementById("hostname").value = data.hostname;
        document.getElementById("ssid").value = data.ssid;
        document.getElementById("password").value = data.password;
        document.getElementById("web_password").value = data.web_password;
        document.getElementById("ap_mode").checked = data.ap_mode;
    });
*/
document.addEventListener("DOMContentLoaded", function () {
    const form = document.getElementById("settings-form");

    form.addEventListener("submit", function (event) {
        event.preventDefault();

        const hostname = document.getElementById("hostname").value;
        const ssid = document.getElementById("ssid").value;
        const password = document.getElementById("password").value;
        const web_password = document.getElementById("web_password").value;
        const ap_mode = document.getElementById("ap_mode").checked;
        const dmx_loop = document.getElementById("dmx_loop").checked;

        // Send the data to your API endpoint using fetch or another AJAX method
        fetch(apiUrl + 'conf', { 
            method: 'POST',
            headers: {'Content-Type': 'application/json'}, 
            body: JSON.stringify({ hostname, ssid, password, web_password, ap_mode, dmx_loop }) 
        }).then(function (response) {
            if (response.ok) {
                setTimeout(function () {
                  window.location.href = "index.html";
              });
                alert("Settings saved successfully!\nDevice restarting in 5 seconds...");
            }
        });
    });
});

function getCookie(name) {
    const cookieName = name + "=";
    const cookieArray = document.cookie.split(";");
    for (let i = 0; i < cookieArray.length; i++) {
      let cookie = cookieArray[i];
      while (cookie.charAt(0) === " ") {
        cookie = cookie.substring(1);
      }
      if (cookie.indexOf(cookieName) === 0) {
        return cookie.substring(cookieName.length, cookie.length);
      }
    }
    return "";
  }
  
  async function authenticatePassword(hashedPassword) {
    if (hashedPassword) {
      // Send a request to the API for password verification using the password cookie
      const response = await fetch(apiUrl + "auth", {
        method: "POST",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify({ password: hashedPassword })
      });
  
      if (response.ok) {
        return;
      } else
        window.location.href = "index.html";
    }
  }
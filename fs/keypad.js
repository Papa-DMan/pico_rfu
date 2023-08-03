let keyBuffer = '';
let prevBuffer = '';
let channelData = new Map();
const displayElement = document.querySelector('.display');
const host = window.location.hostname;
const port = window.location.port;

let apiUrl = `http://${host}:${port}/api/keys`;
let soloMode = false;

const passwordCookie = getCookie("rfu-password");
if (!passwordCookie) {
  window.location.href = "index.html";
}
authenticatePassword(passwordCookie);

function solo() {
  //switch to solo mode where only one value is controlled
  soloMode = !soloMode;
  if (soloMode) {
    keyBuffer = prevBuffer;
    updateDisplay();
    document.getElementById("solo").style.backgroundColor = "red";
  } else {
    document.getElementById("solo").style.backgroundColor = "#e6e6e6";
    release();
  }
}
function soloPlus() {
  //increase the value of the solo mode
  if (soloMode) {
    let match = prevBuffer.match(/\d{3}/)
    let tail = prevBuffer.slice(match.index + 3);
    if (match) {
      keyBuffer = match[0] + " AT 0"
      sendKeys();
      let num = parseInt(match[0]) + 1;
      keyBuffer = num.toString() + tail;
      sendKeys();
      keyBuffer = num.toString() + tail;
      updateDisplay();
    }
  }
}
function soloMinus() {
  if (soloMode) {
    let match = prevBuffer.match(/\d{3}/)
    let tail = prevBuffer.slice(match.index + 3);
    if (match) {
      keyBuffer = match[0] + " AT 0"
      sendKeys();
      let num = parseInt(match[0]) - 1;
      keyBuffer = num.toString() + tail;
      sendKeys();
      keyBuffer = num.toString() + tail;
      updateDisplay();
    }
  }
}

function appendKey(key) {
  // If the user presses the full button twice in a row, send the keys
  if (key === 'FULL' && keyBuffer.endsWith('FULL')) {
    sendKeys();
    return;
  }
  keyBuffer += key;
  updateDisplay();
}

function clearDisplay() {
  keyBuffer = '';
  updateDisplay();
}

function parseBuffer(buffer) {
  let keys = buffer.split(" ");
  let atIndex = keys.indexOf("AT");
  let thruIndex = keys.indexOf("THRU");
  let level = keys[atIndex + 1];
  let channels = [];
  if (level === "FULL" || level === "255") {
    level = "FL";
  }
  if (thruIndex === -1) {
    for (let i = 0; i < atIndex; i++) {
      if (keys[i] === "AND") {
        continue;
      }
      channels.push(keys[i]);
    }
  }
  else {
    for (let i = parseInt(keys[thruIndex - 1]); i <= parseInt(keys[thruIndex + 1]); i++) {
      channels.push(i.toString());
    }
  }
  for (let i = 0; i < channels.length; i++) {
    if (level === "0") {
      console.log("deleting")
      let success = channelData.delete(parseInt(channels[i]).toString());
      console.log(success);
    } else {
      channelData.set(channels[i], level);
    }
  }
  channelData = new Map([...channelData.entries()].sort());
}


function sendKeys() {
  // Pad all the numbers in the keyBuffer with zeros so they are 3 digits long
  var paddedBuffer = keyBuffer.replace(/\b(\d{1,2})(?![\d.])/g, function (match, number) {
    return number.padStart(3, '0');
  });
  prevBuffer = paddedBuffer;
  parseBuffer(keyBuffer);
  // Send the keyBuffer to the backend API


  fetch(apiUrl, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({ keys: paddedBuffer })
  })
    .then(response => response.json())
    .then(data => {
      console.log('API response:', data);
      // Handle the API response as needed
    })
    .catch(error => {
      console.error('Error:', error);
      // Handle any errors that occur during the API call
    });

  clearDisplay();
}

function release() {
  // Send the keyBuffer to the backend API
  channelData.clear();
  soloMode = false;
  document.getElementById("solo").style.backgroundColor = "#e6e6e6";
  fetch(apiUrl, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({ keys: 'release' })
  })
    .then(response => response.json())
    .then(data => {
      console.log('API response:', data);
      // Handle the API response as needed
    })
    .catch(error => {
      console.error('Error:', error);
      // Handle any errors that occur during the API call
    });

  // Clear the keyBuffer and display
  clearDisplay();
  updateChannelDisplay();
}

function updateChannelDisplay() {
  const displayElement = document.querySelector('.channel-display');
  let displayHtml = '<h2>Current Channels and Levels:</h2>';

  if (channelData.length === 0) {
    displayHtml += '<p>No channels to display</p>';
  } else {
    displayHtml += '<div class="channel-array">';

    for (const data of channelData) {
      displayHtml += `
        <div class="channel">
          <p>${data[0]}</p>
          <p>${data[1]}</p>
        </div>
      `;
    }

    displayHtml += '</div>';
  }

  displayElement.innerHTML = displayHtml;
}




function updateDisplay() {
  displayElement.textContent = keyBuffer;
  updateChannelDisplay();
}

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
    const response = await fetch("/api/auth", {
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
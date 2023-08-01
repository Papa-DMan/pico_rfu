let keyBuffer = '';
let prevBuffer = '';
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
  }
}
function soloPlus() {
  //increase the value of the solo mode
  if (soloMode) {
    let match = prevBuffer.match(/\d{3}/)
    let tail = prevBuffer.slice(match.index + 3);
    if (match) {
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

function sendKeys() {
  // Pad all the numbers in the keyBuffer with zeros so they are 3 digits long
  var paddedBuffer = keyBuffer.replace(/\b(\d{1,2})(?![\d.])/g, function(match, number) {
    return number.padStart(3, '0');
  });
  prevBuffer = paddedBuffer;
  
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
}

function updateDisplay() {
  displayElement.textContent = keyBuffer;
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
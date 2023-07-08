let keyBuffer = '';
const displayElement = document.querySelector('.display');
const host = window.location.hostname;
const port = window.location.port;

let apiUrl = `http://${host}:${port}/api/keys`;

const passwordCookie = getCookie("rfu-password");
if (!passwordCookie) {
  window.location.href = "index.html";
}



function appendKey(key) {
  // If the user presses the full button twice in a row, send the keys
  if (key === 'full' && keyBuffer.endsWith('full')) {
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

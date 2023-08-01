let patch = [];

// Function to generate a vertical fader element
function createFader(id) {
  const faderContainer = document.createElement('div');
  faderContainer.classList.add('fader-wrapper');

  const fader = document.createElement('input');
  fader.type = 'range';
  fader.min = 0;
  fader.max = 100;
  fader.value = 0;
  fader.classList.add('fader');
  fader.oninput = () => updateFader(id, fader.value);
  fader.id = `fader-${id}`;

  const label = document.createElement('span');
  label.textContent = `Fader ${id}`;
  label.classList.add('fader-label');

  faderContainer.appendChild(fader);
  faderContainer.appendChild(label);

  return faderContainer;
}

// Function to update the fader value and send the DMX data to the API
function updateFader(id, value) {
  // Calculate the DMX data to be sent (assuming the API expects values from 0 to 255)
  const dmxValue = Math.round(value * 2.55);
  channel = patch[id - 1];
  sendKeys(`${channel} AT ${dmxValue}`);
  // Replace this with the code to send the DMX data to the API
  console.log(`Sending Channel ${channel} DMX value: ${dmxValue}`);
}

// Create 10 vertical faders (you can adjust this number)
let channel = 1;
const faderContainers = document.querySelectorAll('.fader-container');
for (let faderContainer of faderContainers) {
  for (let i = 0; i < 10; i++, channel++) {
    faderContainer.appendChild(createFader(channel));
  }
}


function setCookie(name, value) {
  document.cookie = `${name}=${encodeURIComponent(value)}; expires=Fri, 31 Dec 9999 23:59:59 GMT; path=/`;
}

function getCookie(name) {
  const cookies = document.cookie.split(';');
  for (const cookie of cookies) {
    const [cookieName, cookieValue] = cookie.split('=');
    if (cookieName.trim() === name) {
      return decodeURIComponent(cookieValue);
    }
  }
  return null;
}


/* Patch code */

function loadPatch() {
  const savedPatch = getCookie('console_patch');

  if (savedPatch) {
    console.log('Loading saved patch:', savedPatch);
    patch = JSON.parse(savedPatch);
  }
  else {
    console.log("No saved patch found.");
  }
}

function savePatch() {
  setCookie('console_patch', JSON.stringify(patch));
}

let selectedFader = null;

function toFader() {
  if (selectedFader !== null) {
    if (pastBuffer !== '') {
      patch[selectedFader] = pastBuffer;
      savePatch();
      console.log('Saved patch:', patch);
      pastBuffer = '';
      selectedFader = null;
    }
  } else {
    selectedFader = prompt("Select a fader number to assign to: ") - 1;
    if (selectedFader < 0 || selectedFader > document.getElementsByClassName('fader').length) {
      selectedFader = null;
      alert("Invalid fader number.");
    }
    toFader();
  }
}



/* Keypad code */

let keyBuffer = '';
let pastBuffer = '';
const displayElement = document.querySelector('.display');
const host = window.location.hostname;
const port = window.location.port;

let apiUrl = `http://${host}:${port}/api/keys`;

function appendKey(key) {
  // If the user presses the full button twice in a row, send the keys
  if (key === 'FULL' && keyBuffer.endsWith('FULL')) {
    sendKeys(keyBuffer);
    return;
  }
  keyBuffer += key;
  updateDisplay();
}

function clearDisplay() {
  keyBuffer = '';
  updateDisplay();
}

function sendKeys(outputBuffer) {
  // Pad all the numbers in the keyBuffer with zeros so they are 3 digits long
  var paddedBuffer = outputBuffer.replace(/\b(\d{1,2})(?![\d.])/g, function (match, number) {
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
  pastBuffer = keyBuffer;
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

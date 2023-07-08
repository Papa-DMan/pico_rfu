// Check if the password cookie exists
const passwordCookie = getCookie("password");
if (passwordCookie) {
  // If the cookie exists, automatically fill the password input
  document.getElementById("password").value = passwordCookie;
}

// Function to handle form submission
async function submitForm(event) {
  event.preventDefault(); // Prevent the form from being submitted

  // Get the password value entered by the user
  const passwordInput = document.getElementById("password").value;

  // Send a request to the API for password verification
  const response = await fetch("/api/auth", {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify({ password: passwordInput })
  });

  if (response.ok) {
    // Set a cookie to remember the password
    setCookie("password", passwordInput, 7); // Cookie expires in 7 days

    // Forward to the keypad page
    window.location.href = "keypad.html";
  } else {
    // Display an error message or take appropriate action
    alert("Incorrect password. Please try again.");
  }
}

// Function to set a cookie
function setCookie(name, value, days) {
  const date = new Date();
  date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000)); // Set expiry date
  const expires = "expires=" + date.toUTCString();
  document.cookie = name + "=" + value + ";" + expires + ";path=/";
}

// Function to get a cookie by name
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

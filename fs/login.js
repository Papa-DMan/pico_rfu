document.addEventListener("DOMContentLoaded", function() {
    // Function to handle form submission
    async function submitForm(event) {
      event.preventDefault(); // Prevent the form from being submitted
  
      // Get the password value entered by the user
      const passwordInput = document.getElementById("password").value;
      const encryptedPassword = await encryptPassword(passwordInput);
      // Send a request to the API for password verification
      const response = await fetch("/api/auth", {
        method: "POST",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify({ password: encryptedPassword })
      });
  
      if (response.ok) {
        // Set a cookie to remember the password
        setCookie("rfu-password", encryptedPassword, 7); // Cookie expires in 7 days
  
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
  
    // Check if the password cookie exists
    const passwordCookie = getCookie("rfu-password");
  
    // Re-authenticate the password cookie with the API
    async function authenticatePassword() {
      if (passwordCookie) {
        // Send a request to the API for password verification using the password cookie
        const response = await fetch("/api/auth", {
          method: "POST",
          headers: {
            "Content-Type": "application/json"
          },
          body: JSON.stringify({ password: passwordCookie })
        });
  
        if (response.ok) {
          // Forward to the keypad page if the cookie is authenticated
          window.location.href = "keypad.html";
        }
      }
    }
  
    authenticatePassword();
  
    // Attach the form submission handler
    const form = document.getElementById("login-form");
    form.addEventListener("submit", submitForm);
  });
  

  async function encryptPassword(password) {
    const publicKeyPath = '/public.pem';  // Replace with the correct path to your public key file
  
    // Fetch the public key from the PEM file
    const response = await fetch(publicKeyPath);
    const publicKeyPem = await response.text();
    const publicKey = await crypto.subtle.importKey(
      'spki',
      convertPemToBinary(publicKeyPem),
      {
        name: 'RSA-OAEP',
        hash: { name: 'SHA-256' },
      },
      true,
      ['encrypt']
    );
  
    // Convert the password string to an ArrayBuffer
    const encoder = new TextEncoder();
    const passwordBuffer = encoder.encode(password + "salt=" + toString(Math.random() * (64 - password.length)));
    
  
    // Encrypt the password using the public key
    const encryptedPasswordBuffer = await crypto.subtle.encrypt(
      {
        name: 'RSA-OAEP',
      },
      publicKey,
      passwordBuffer
    );
  
    // Convert the encrypted password to a base64-encoded string
    const encryptedPasswordArray = Array.from(new Uint8Array(encryptedPasswordBuffer));
    const encryptedPasswordBase64 = btoa(String.fromCharCode.apply(null, encryptedPasswordArray));
  
    return encryptedPasswordBase64;
  }
  
  function convertPemToBinary(pem) {
    const pemHeader = '-----BEGIN PUBLIC KEY-----';
    const pemFooter = '-----END PUBLIC KEY-----';
    const pemContents = pem.replace(pemHeader, '').replace(pemFooter, '').trim();
    const binaryString = window.atob(pemContents);
    const binaryLen = binaryString.length;
    const bytes = new Uint8Array(binaryLen);
    for (let i = 0; i < binaryLen; ++i) {
      bytes[i] = binaryString.charCodeAt(i);
    }
    return bytes.buffer;
  }
  
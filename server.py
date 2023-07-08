from flask import Flask, request, send_from_directory

# Create an instance of the Flask application
app = Flask(__name__)

# Define a route for the API endpoint
@app.route("/api/auth", methods=["POST"])
def handle_auth():
    # Get the password from the request body
    password = request.json.get("password")

    if password is None:
        return "No password provided", 400
    if not isinstance(password, str):
        return "Password must be a string", 400
    if password == "12345678": 
        return "Password verified"
    return "Incorrect password", 401

@app.route("/api/keys", methods=["POST"])
def handle_keys():
    # Get the password from the request body
    keys = request.json.get("keys")
    return "Keys verified", 200
    


# Define a route to serve static files from the "fs" folder
@app.route("/<path:filename>")
def serve_static(filename):
    return send_from_directory("fs", filename)

@app.route("/")
def serve_index():
    return send_from_directory("fs", "index.html")

# Run the Flask application
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)

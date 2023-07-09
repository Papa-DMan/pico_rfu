Place a private key pem file without a password here

When generating your website's fsdata the public key should be included in your fs directory.

Example generation command:
openssl genrsa -des3 -out private.pem 2048
openssl rsa -in private.pem -outform PEM -pubout -out public.pem
openssl rsa -in private.pem -out private_unencrypted.pem -outform PEM
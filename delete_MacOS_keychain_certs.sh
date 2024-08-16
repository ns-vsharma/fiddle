#!/bin/bash

# SHA-1 hash of the certificate to delete
CERTIFICATE_SHA1="13B388A1C5506BCC7A21A1C718B7EF96747D89FA"

# Get a list of all users on the system
USER_LIST=$(dscl . list /Users | grep -v '^_')

# Function to delete certificate from a given keychain using SHA-1
delete_certificate() {
    local KEYCHAIN=$1
    local CERT_SHA1=$2
    local USER=$3
    if [ -n "$USER" ]; then
    	echo "Deleting certificate with SHA-1 '$CERT_SHA1' from keychain '$KEYCHAIN' for user $USER"
        # If USER is provided, run as that user
        sudo -u "$USER" security delete-certificate -Z "$CERT_SHA1" "$KEYCHAIN"
    else
    	echo "Deleting certificate with SHA-1 '$CERT_SHA1' from keychain '$KEYCHAIN'"
        # If no USER is provided, delete from system keychain
        sudo security delete-certificate -Z "$CERT_SHA1" "$KEYCHAIN"
    fi
}

# Delete matching cert fron all user keychains
for USER in $USER_LIST; do
    USER_KEYCHAIN="/Users/$USER/Library/Keychains/login.keychain-db"
    if [ -f "$USER_KEYCHAIN" ]; then
        delete_certificate "$USER_KEYCHAIN" "$CERTIFICATE_SHA1" "$USER"
    else
        #echo "Keychain not found for user $USER"
    fi
done

SYSTEM_KEYCHAINS=(
    "/Library/Keychains/System.keychain"
    #"/Library/Keychains/SystemRootCertificates.keychain"
)

# Delete matching cert fron all system keychains
for KEYCHAIN in "${SYSTEM_KEYCHAINS[@]}"; do
    if [ -f "$KEYCHAIN" ]; then
        delete_certificate "$KEYCHAIN" "$CERTIFICATE_SHA1"
    else
        #echo "System keychain '$KEYCHAIN' not found"
    fi
done

echo "Certificate deletion process successful"

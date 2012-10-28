# FiSHLiM

FiSHLiM is an XChat plugin for FiSH IRC encryption. It's my attempt at making a simple, lightweight and secure plugin for this encryption protocol. For more info, please visit the [FiSHLiM website](http://fishlim.kodafritt.se/).

For installation instructions, see the INSTALL file in the sources.

## Features

Working:

 * Sending/receiving messages
 * Topic decryption
 * Using unecrypted keys / keys without a password from blow.ini
 * Pure protocol-level filtering (works with highlighting, nick coloring etc)
 * Partially encrypted messages (i.e. prefixed with nickname by a bouncer)

Not working:

 * Key exchange
 * Password-protected key storage
 * Topic encryption
 * Remote exploitation (hopefully!)
 * Plaintext content that contain +OK is decrypted twice

## Commands

Keys are stored in the configuration file in ~/.config/hexchat/addon_fishlim.txt. To set the encryption key for the nick or channel to password:

<pre>/setkey  [nick or #channel]  password</pre>

To delete the given nick or channel from the configuration file:

<pre>/delkey  nick-or-#channel</pre>

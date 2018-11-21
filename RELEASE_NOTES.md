# Release Notes

## 2.0.0

- Totally refactored socket connection. Now avaiable switch ssl|nossl server just using config.
- Separate SSL enabling for REST and WebSocket server
- Adding client connection only after successfully authorizing. This means, user no more have ability to send message while autorizing in progress, especcialy for "remote-auth"
- Event notifier optimizing. Using condition_variable, it doesn't check more for new events every 0.5 seconds, only when new message has added to queue or every N seconds (retry interval for failed events)
- Updated dependencies
- Bundled openssl 1.1.1 (x86-64 linux or darwin). Sometimes with old version, server segfaulted while handshaking with simple "https" requests

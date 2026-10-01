# kioskcnc

2026-10-01

Daemon for ad-hoc operations on my convention kiosks.
Watch for changes to known saved-game files, notify our server.

The saved games we monitor are hard-coded in `savewatch.c`.

## TODO

- [ ] Log changed save files locally.
- [ ] Send changed save files to a server.
- [ ] Write that server.
- - [x] Bare minimum POC: Node server. Receive save files from an external client and indicate their reception in a web app. No need to do anything with the data.
- - [x] Receive files.
- - [x] Log files here too.
- - [ ] Display news and stats in a big friendly presentation suitable for my big TV.
- - [ ] Generate saved-game links for Bellacopia, and let me copy them from my phone to email to users.

## Client/Server

Clients run on each station. So one each on the three laptops and one on the cabinet.

Server will drive the TV and should also be accessible for miscellaneous use throughout the show eg taking notes.
Is the Macbook a good choice for that? It's easy to carry at least.

Do the display bit as a web app, and all the comms and logic in a Node server?
Validate first that the Macbook can handle all that: Node server, drive the TV with a fullscreen web app.
Ideally I'll be able to talk to it via Smartphone too -- can the Macbook's radio do this?
- Displaying a fullscreen web app via HDMI while using the laptop as usual: No problem.
- Node v13 is present, should suffice for a server.
- I'm able to set up an access point from the Macbook's wifi, but DHCP doesn't seem to work. Phone never acquires an IP address.
- - This might not matter for 2026: Rumor says there will be free wifi on the floor. Maybe no need for a phone.

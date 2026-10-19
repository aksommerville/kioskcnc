# kioskcnc

2026-10-01

Daemon for ad-hoc operations on my convention kiosks.
Watch for changes to known saved-game files, notify our server.

The saved games we monitor are hard-coded in `savefiles.c`.

In just one evening of testing, I generated over 1 MB of log files. Bellacopia in particular, makes a file every time anything changes.
This is fine for GDEX. I mean, if there's 100 MB of logs, I can work with it.
But if we do something like this more permanent, might need to get smarter about packing data.

Update 2026-10-05
Alas everything has gone to shit.
I got 3 USB Ethernet adapters (Realtek RTL8153 inside), and can't get them working with the contops.
Same deal with their radios even.
So I think we're not going to do the big scoreboard.
Instead, modify the kioskcnc client to dump files in `data`, and at least I can analyze them after.
Keep the existing `output` unit in place. Add an alternative to it, create one or the other at startup, and have `savewatch` send to the one that exists.

## TODO

- [ ] Use this project for high-level kiosk configuration. eg orchestration clone and build.
- - Eventually ra4 should relieve us of some of that. But if there's opportunity for lightweight conveniences in the near term...
- - Making an Egg change on a kiosk is kind of painful because you have to rebuild all the games. A script in here could ease that.

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

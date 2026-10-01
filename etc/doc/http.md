# kioskcnc http interface

All GET requests are for static content, from `src/www/`.

All API requests are POST.

We have two flavors of client: The endpoint kiosks sending us logs, and the presumably-local web app polling for those logs.

Turn everything into text on the wire.
For Egg saved games, that means the kiosk should decode the binary kv format and turn it into a JSON string:string object.

--

`POST /poll`

Send any events received since the last poll.
If there's more than one client calling poll, they'll each see a different incomplete stream of events.
We're built for one polling client.

May block for a tasteful interval if nothing is queued.

Request body ignored.

Response is JSON:
```
{
  events: {
    file: string
    body: {...}
  }[]
}
```

--

`POST /event`

Client delivers a new event.

Request:
```
{
  file: string
  body: {...}
}
```

No meaningful response.

--

`POST /getall`

Like `POST /poll`, but send the entire current state.

--

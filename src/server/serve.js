const fs = require("fs");
const scoreboard = require("./scoreboard");

/* Guess MIME type.
 * Because browsers aren't big on common sense.
 */

function guessMimeType(serial, path) {
  // We could look for signatures in (serial) but that's overkill.
  // We control the content completely. So just stick to common path extensions and depend on those.
  const match = path.match(/^.*\.([^.]*)$/);
  if (!match) return "application/octet-stream";
  switch (match[1].toLowerCase()) {
    case "html": return "text/html";
    case "ico": return "image/x-icon";
    case "png": return "image/png";
    case "css": return "text/css";
    case "js": return "application/javascript";
  }
  return "application/octet-stream";
}

/* Serve some file, confirmed to exist.
 */

function serveRegularFile(req, rsp, path) {
  const serial = fs.readFileSync(path);
  const type = guessMimeType(serial, path);
  rsp.statusCode = 200;
  rsp.setHeader("Content-Type", type);
  rsp.end(serial);
}

/* Serve a directory.
 */

function serveDirectory(req, rsp, path) {
  return respondError(rsp, 404); // ...nah we don't need this
}

/* Serve static content.
 */

function serveStatic(req, rsp, htdocs) {
  const rpath = req.url.split("?")[0];
  if (!rpath.startsWith("/") || (rpath.indexOf("..") >= 0)) return respondError(rsp, 404);
  const lpath = htdocs + rpath;
  try {
    const st = fs.statSync(lpath);
    if (st.isDirectory()) {
      const indexPath = lpath + "/index.html";
      try {
        const st2 = fs.statSync(indexPath);
        if (st2.isFile()) {
          return serveRegularFile(req, rsp, indexPath);
        }
      } catch (e) {
        console.log(e);
      }
      return serveDirectory(req, rsp, lpath);
    }
    if (st.isFile()) {
      return serveRegularFile(req, rsp, lpath);
    }
    return respondError(rsp, 404);
  } catch (e) {
    console.log(e);
    return respondError(rsp, 404);
  }
}

/* Respond with a generic error.
 */

function respondError(rsp, status, msg) {
  rsp.statusCode = status;
  rsp.statusMessage = msg || "Error";
  rsp.end();
}

/* Some global state for poll and event.
 */

const pendingPolls = []; // HTTP Response
const pendingEvents = []; // {file,body}
let pollFlushTimeout = null;

function schedulePollFlush() {
  if (pollFlushTimeout) return;
  pollFlushTimeout = setTimeout(() => {
    pollFlushTimeout = null;
    const body = JSON.stringify({
      events: pendingEvents, // Really should be empty; an incoming event would flush it. But let's be safe.
    });
    for (let i=pendingPolls.length; i-->0; ) {
      pendingPolls[i].statusCode = 200;
      pendingPolls[i].setHeader("Content-Type", "application/json");
      pendingPolls[i].end(body);
    }
    pendingPolls.splice(0, pendingPolls.length);
    pendingEvents.splice(0, pendingEvents.length);
  }, 5000);
}

function unschedulePollFlush() {
  if (!pollFlushTimeout) return;
  clearTimeout(pollFlushTimeout);
  pollFlushTimeout = null;
}
    

/* POST /poll
 */

function servePoll(req, rsp) {
  if (pendingEvents.length) {
    const body = JSON.stringify({
      events: pendingEvents,
    });
    pendingEvents.splice(0, pendingEvents.length);
    rsp.statusCode = 200;
    rsp.setHeader("Content-Type", "application/json");
    rsp.end(body);
  } else {
    pendingPolls.push(rsp);
    schedulePollFlush();
  }
}

/* POST /event
 */

function serveEvent(req, rsp, body) {
  try {
    body = JSON.parse(body);
    scoreboard.addFile(body.host, body.file, body.body);

    if (pendingPolls.length) {
      for (let i=pendingPolls.length; i-->0; ) {
        pendingPolls[i].statusCode = 200;
        pendingPolls[i].setHeader("Content-Type", "application/json");
        pendingPolls[i].end(JSON.stringify({ events: [body] }));
      }
      pendingPolls.splice(0, pendingPolls.length);
      unschedulePollFlush();

    } else {
      pendingEvents.push(body);

    }
  } catch (e) {
    console.error(e);
    return respondError(rsp, 404);
  }
  rsp.statusCode = 200;
  rsp.end();
}

/* POST /getall
 */

function serveGetall(req, rsp) {
  rsp.statusCode = 200;
  rsp.setHeader("Content-Type", "application/json");
  rsp.end(JSON.stringify({ events: scoreboard.getAll() }));
}

/* Serve HTTP request, main entry point.
 */

module.exports = function serve(req, rsp, htdocs) {
  //console.log(`serve: ${req.method} ${req.url}`);
  let body = "";
  req.on("data", (d) => body += d);
  req.on("end", () => {
    switch (req.method) {
      case "GET": {
          if (!htdocs) return respondError(rsp, 404);
          return serveStatic(req, rsp, htdocs);
        }
      case "POST": {
          let sepp = req.url.indexOf("?");
          if (sepp < 0) sepp = req.url.length;
          const path = req.url.substring(0, sepp);
          const query = req.url.substr(sepp + 1).split('&').map(src => {
            let fsepp = src.indexOf("=");
            if (fsepp < 0) fsepp = src.length;
            const k = decodeURIComponent(src.substring(0, fsepp));
            const v = decodeURIComponent(src.substr(fsepp + 1));
            return [k, v];
          }).reduce((a, v) => {
            if (v[0]) a[v[0]] = v[1];
            return a;
          }, {});
          //console.log(`POST ${JSON.stringify(path)} ? ${JSON.stringify(query)}`);
          switch (path) {
            case "/poll": return servePoll(req, rsp);
            case "/event": return serveEvent(req, rsp, body);
            case "/getall": return serveGetall(req, rsp);
            default: return respondError(rsp, 404);
          }
        }
      default: return respondError(rsp, 404);
    }
  });
}

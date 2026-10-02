/* KioskService.js
 * Responsible for communication with our server, and stashing data from it.
 * Our responsibility ends with collecting the files.
 *
 * We do distinguish stateful from hiscore files because that's a storage concern, but otherwise we don't know about file content.
 * "stateful": Proper saved games, such that the interesting things to report can only be derived by diffing. Bellacopia.
 * "hiscore": Assume that we're going to pick just one of the file's states, the best score, and report that permanently. Most games.
 */

/* Any file name listed here will be tracked per host.
 * As opposed to yoinking the high score across all hosts.
 * The idea is we don't really have a concept of "high score" for Bellacopia Maleficia,
 * but we do want to show like "somebody got the broom!" in real time.
 */
const STATEFUL_FILES = [
  "bellacopia",
];
 
export class KioskService {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.pollTimeout = null; // Stays true while the poll call is in flight.
    this.cancelSeq = 0; // Increment when refreshing. Poller discards response if it changes.
    this.scores = []; // {host,file,body} straight off the wire.
    this.states = {}; // Key is hostname, value is an object where key is file and value that file's state (another object).
    this.nextListenerId = 1;
    this.listeners = []; // {id,cb}
    
    this.refresh();
  }
  
  /* (cb) will be called whenever something changes.
   *   { action: "add", score: {host,file,body}}
   *   { action: "update", score: {host,file,body}} Score files only.
   *   { action: "remove", score: {host,file,body}}
   *   { action: "change", score: {host,file,body,old}} Stateful files only.
   * Our states are fully reduced before any notification is fired.
   * Except when refreshing, there's one pass of "remove" followed by a pass of "add".
   */
  listen(cb) {
    const id = this.nextListenerId++;
    this.listeners.push({ id, cb });
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) {
      this.listeners.splice(p, 1);
    }
  }
  
  broadcast(event) {
    for (const { cb } of this.listeners) cb(event);
  }
  
  iterate(cb) {
    for (const score of this.scores) cb(score);
    for (const host of Object.keys(this.states)) {
      for (const file of Object.keys(this.states[host])) {
        cb({ host, file, score: this.states[host][file] });
      }
    }
  }
  
  refresh() {
    if (this.pollTimeout) {
      this.window.clearTimeout(this.pollTimeout);
      this.pollTimeout = null;
      this.cancelSeq++;
    }
    this.replaceContent(null);
    const cancelSeq = this.cancelSeq;
    this.window.fetch("/getall", { method: "POST" }).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.json();
    }).then(rsp => {
      if (this.cancelSeq !== cancelSeq) throw new Error("Cancelled");
      //console.log(`got full response`, rsp);
      this.replaceContent(rsp);
      this.poll();
    }).catch(e => {
      this.window.console.error(`getall failed`, e);
    });
  }
  
  poll() {
    if (this.pollTimeout) return;
    const cancelSeq = this.cancelSeq;
    this.pollTimeout = this.window.setTimeout(() => {
      this.window.fetch("/poll", { method: "POST" }).then(rsp => {
        if (!rsp.ok) throw rsp;
        return rsp.json();
      }).then(rsp => {
        if (this.cancelSeq !== cancelSeq) throw new Error("Cancelled");
        //console.log(`got poll response`, rsp);
        this.pollTimeout = null;
        this.appendContent(rsp, true);
        this.poll();
      }).catch(e => {
        this.window.console.error(`poll failed`, e);
        this.pollTimeout = null;
      });
    }, 1000);
  }
  
  replaceContent(rsp) {
  
    const rmscores = this.scores;
    const rmstates = this.states;
    this.scores = [];
    this.states = {};
    for (const score of rmscores) this.broadcast({ action: "remove", score });
    for (const host of Object.keys(rmstates)) {
      for (const file of Object.keys(host)) {
        this.broadcast({ action: "remove", score: { host, file, body: host[file] }});
      }
    }
    
    this.appendContent(rsp, false);
    
    for (const score of this.scores) this.broadcast({ action: "add", score });
    for (const host of Object.keys(this.states)) {
      for (const file of Object.keys(host)) {
        this.broadcast({ action: "add", score: { host, file, body: host[file] }});
      }
    }
  }
  
  appendContent(rsp, notify) {
    if (!(rsp?.events instanceof Array)) return;
    const added=[], updated=[], changed=[];
    for (const event of rsp.events) {
      const sane = this.sanitizeScore(event);
      if (!sane) continue;
      if (STATEFUL_FILES.indexOf(sane.file) >= 0) {
        if (!this.states[sane.host]) this.states[sane.host] = {};
        const old = this.states[sane.host][sane.file];
        if (old) changed.push({ ...sane, old });
        else added.push(sane);
        this.states[sane.host][sane.file] = sane.body;
      } else {
        this.scores.push(sane);
        added.push(sane);
      }
    }
    if (notify) {
      for (const score of added) this.broadcast({ action: "add", score });
      for (const score of updated) this.broadcast({ action: "update", score });
      for (const score of changed) this.broadcast({ action: "change", score });
    }
  }
  
  /* Returns null, (src), or a sanitized copy of (src).
   */
  sanitizeScore(src) {
    // Must be an object.
    if (!src) return null;
    if (typeof(src) !== "object") return null;
    // Must have a valid file name.
    if (!src.file?.match?.(/^[a-zA-Z0-9_]+$/)) return null;
    // Body must be an object.
    if (!src.body || (typeof(src.body) !== "object") || (src.body instanceof Array)) return null;
    // We can make up a host if needed.
    if (src.host?.match?.(/^[a-zA-Z0-9_]+$/)) return src;
    return { host: "unknown", file: src.file, body: src.body };
  }
}

KioskService.singleton = true;

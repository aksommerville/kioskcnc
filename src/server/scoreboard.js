/* scoreboard.js
 * Responsible for decoding, saving, and loading high scores.
 */

const fs = require("fs");

/* Global scoreboard.
 */

const hiscores = {
  all52: null, // "hiscore", completion time, decimal milliseconds.
  apothecary: null, // "hiscore", completion time, decimal milliseconds.
  // bellacopia doesn't participate
  cherteau: null, // "hiscore". 9:time(MM:SS.mmm) 3:winc 3:battlec 3:gold 3:danceoff.
  // deadweight doesn't save a high score. it should!
  // fullmoon4 has a save file but i'm durned if i can remember how it's structured. Probly not useful for score reporting anyway.
  hummfu: null, // "hiscore", 6 digits.
  iggle: null, // "hiscore", completion time, decimal milliseconds.
  inversion: null, // "hiscore", 6 digits.
  kabobblin: null, // "hiscore". 8 digits. (max possible score is 2300, why are we using 8 digits?)
  kleptomania: null, // "hiscore_100", "hiscore_any". Both 9 digit completion times "MM:SS.mmm". Dashless time doesn't get recorded.
  licensetoilluse: null, // "hiscore": 6 digits, "besttime": MM:SS.mmm
  // lilsitter has a "hiscore" file but not sure of the format. It records a time for each level.
  // myscrypt has a save file but no score worth reporting.
  penance: null, // "besttime": MM:SS.mmm
  // pokorc I'm not sure where it saves, or the format. Might be nice to have.
  presto: null, // "hiscore": 6 digits.
  queenofclocks: null, // "hiscore", 6 digits.
  samsam: null, // "hiscore": "COINC,MM:SS.mmm"
  // sitter2009 has lots of score storage, but no straightforward single value.
  // spellingbee i'm not sure what we should track. One doesn't typically play the whole way thru (I don't think anyone ever has, at an expo).
  upsy: null, // "hiscore", 4 digits.
  vexularg: null, // "hiscore", 6 digits.
  wicked: null, // "hiscore". A few space-delimited integers, not sure what they mean.
  wishbone: null, // "save": Lots of things and ";MS" at the end.
  xrm: null, // "hiscore". Semicolon-delimited fields, the interesting one is "ot=MM:SS.mmm"
  younap: null, // "hiscore". "SCORE;MM:SS.mmm;MM:SS.mmm" first time is Any%, second is 100%.
  zennoniwa: null, // "hiscore" "MM:SS.mmm;NUMBER;NUMBER", not sure what those other two things are.
};

/* Check an incoming file and capture it if it's better than our current high score.
 */

function possibleHighScore(host, name, body) {
  //...well. Actually I'm not sure it's worth recording these in the server.
  // Why not let the web app pull them all down and figure it out on its own?
  // I know that becomes grossly inefficient once the set is well populated, but it will be a new set every weekend. who cares.
}

/* Return the full set of events, one per file.
 */

function getAll() {
  const events = [];
  for (const base of fs.readdirSync("data")) {
    const match = base.match(/^\d{14}-([a-zA-Z0-9_]+)-([a-zA-Z0-9]+)\.json$/);
    if (!match) continue;
    try {
      const host = match[1];
      const name = match[2];

      // Bellacopia records state changes pretty often. There are going to be hundreds of files.
      // And that's by design. I do want to be able to reconstruct user sessions from their various state files.
      // But it's ridiculous to send these all to the web app, when we're not even capturing a "high score" for it.
      if (name === "bellacopia") continue;
      
      const body = JSON.parse(fs.readFileSync("data/" + base).toString("utf8"));
      events.push({ host, name, body });
    } catch (e) {
      console.log(`${base}:ERROR: ${e.message}`);
    }
  }
  return events;
}

/* Receive an event.
 */

function addFile(host, name, body) {
  if (!name || !body) return;
  name = name.replace(/[^a-zA-Z0-9].*$/, "");
  if (!name) return;
  if (!host) host = "unknown";
  const now = new Date();
  const prefix = now.getFullYear().toString() +
    (now.getMonth() + 1).toString().padStart(2, '0') +
    now.getDate().toString().padStart(2, '0') +
    now.getHours().toString().padStart(2, '0') +
    now.getMinutes().toString().padStart(2, '0') +
    now.getSeconds().toString().padStart(2, '0');
  const path = `data/${prefix}-${host}-${name}.json`;
  fs.writeFileSync(path, JSON.stringify(body) + "\n");
  possibleHighScore(host, name, body);
}

/* Module definition.
 */

module.exports = {
  addFile,
  getAll,
};

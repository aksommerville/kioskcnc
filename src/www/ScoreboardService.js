/* ScoreboardService.js
 * Tracks content shaped for reporting to the user.
 * We read events from KioskService and compose a logical model from them.
 */
 
import { KioskService } from "./KioskService.js";

export class ScoreboardService {
  static getDependencies() {
    return [KioskService];
  }
  constructor(kioskService) {
    this.kioskService = kioskService;
    
    this.nextListenerId = 1;
    this.listeners = []; // {cb,id}
    this.scores = {}; // Key is file name, value is score formatted for display.
    this.states = {}; // Key is file name, value is an object keyed by host, whose values are a per-game schema.
    
    this.kioskListener = this.kioskService.listen(e => this.onKioskEvent(e));
    this.kioskService.iterate(e => this.onKioskEvent({ action: "add", score: e }));
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) this.listeners.splice(p, 1);
  }
  
  /* cb() will be called with one of:
   *   { action: "hiscore", file: string, score: string } # Formatted for display, and confirmed to be a new record. (file) might be new.
   *   { action: "pity", file: string, score: string } # Fired instead of "hiscore", when we got a new one but it's not a record.
   *   { action: "item", file: "bellacopia", name: string }
   *   { action: "rootdevil", file: "bellacopia", id: number }
   *   { action: "treestory", file: "bellacopia", treeid: number, story: string } # We provide a story name for display.
   *   { action: "zoo", file: "bellacopia", zoo: string, monster: string }
   * TODO We can add all kinds of Bellacopia events. Think on what's desired.
   */
  listen(cb) {
    const id = this.nextListenerId++;
    this.listeners.push({ cb, id });
    return id;
  }
  
  broadcast(event) {
    for (const { cb } of this.listeners) cb(event);
  }
  
  /* Scoring, with per-game knowledge.
   ***********************************************************************************************/
  
  onKioskEvent(event) {
    //console.log(`ScoreboardService.onKioskEvent`, { event, kiosk: this.kioskService });
    switch (event.action) {
      case "add": case "update": case "change": {
          switch (event.score.file) {
            case "all52": this.receiveScore(event.score.file, event.score.body.hiscore, "nopuncttime"); break;
            case "apothecary": this.receiveScore(event.score.file, event.score.body.hiscore, "ms"); break;
            case "bellacopia": this.receiveState(event.score.file, event.score.host, event.score.body.save); break;
            case "cherteau": this.receiveScore(event.score.file, event.score.body.hiscore); break;
            case "hummfu": this.receiveScore(event.score.file, event.score.body.hiscore, "decimal"); break;
            case "iggle": this.receiveScore(event.score.file, event.score.body.highscore, "ms"); break;
            case "inversion": this.receiveScore(event.score.file, event.score.body.hiscore, "decimal"); break;
            case "kabobblin": this.receiveScore(event.score.file, event.score.body.hiscore, "decimal"); break;
            case "kleptomania": this.receiveScore(event.score.file, event.score.body.hiscore_any || event.score.body.hiscore_100, "time"); break; // Also has (hiscore_100), same format. TODO?
            case "licensetoilluse": this.receiveScore(event.score.file, event.score.body.hiscore, "decimal"); break; // Also has (besttime) fmt "time". TODO?
            case "penance": this.receiveScore(event.score.file, event.score.body.besttime, "time"); break;
            case "presto": this.receiveScore(event.score.file, event.score.body.hiscore, "decimal"); break;
            case "queenofclocks": this.receiveScore(event.score.file, event.score.body.hiscore, "decimal"); break;
            //case "samsam": this.receiveScore(event.score.file, event.score.body.hiscore); break; // XXX Eliminated to create a 3x5 grid, and also who cares.
            case "upsy": this.receiveScore(event.score.file, event.score.body.hiscore, "decimal"); break;
            case "vexularg": this.receiveScore(event.score.file, event.score.body.hiscore, "decimal"); break;
            case "wicked": this.receiveScore(event.score.file, event.score.body.hiscore); break;
            //case "wishbone": this.receiveScore(event.score.file, event.score.body.save); break; // XXX Eliminated. It saves repeatedly, so the recorded time isn't necessarily completion time.
            case "xrm": this.receiveScore(event.score.file, event.score.body.hiscore); break;
            case "younap": this.receiveScore(event.score.file, event.score.body.hiscore); break;
            case "zennoniwa": this.receiveScore(event.score.file, event.score.body.hiscore); break;
            default: console.log(`ScoreboardService: Unknown game ${JSON.stringify(event.score.file)}`); break;
          }
        } break;
    }
  }
  
  receiveScore(file, src, fmt) {
    if (!fmt) fmt = file;
    const prev = this.scores[file];
    //console.log(`ScoreboardService.receiveScore file=${JSON.stringify(file)} fmt=${JSON.stringify(fmt)} src=${JSON.stringify(src)} prev=${JSON.stringify(prev)}`);
    let score = null;
    let cmp = () => 0;
    switch (fmt) {
      // Common generic formats:
      case "ms": {
          score = this.reprMs(src);
          cmp = (a, b) => this.cmpTime(a, b);
        } break;
      case "decimal": {
          score = this.sanitizeDecimal(src);
          cmp = (a, b) => this.cmpDecimal(a, b);
        } break;
      case "nopuncttime": {
          score = this.sanitizeNopunctTime(src);
          cmp = (a, b) => this.cmpTime(a, b);
        } break;
      case "time": {
          score = this.sanitizeTime(src);
          cmp = (a, b) => this.cmpTime(a, b);
        } break;
      // Odd formats peculiar to just one game each:
      case "cherteau": {
          // 9:time(MM:SS.mmm) 3:winc 3:battlec 3:gold 3:danceoff. We have to reduce to a scalar, so let's use time.
          score = this.sanitizeTime(src.substring(0, 9));
          cmp = (a, b) => this.cmpTime(a, b);
        } break;
      case "samsam": {
          // "COINC,MM:SS.mmm". Let's use coinc.
          score = this.sanitizeDecimal(src.split(",")[0]);
          cmp = (a, b) => this.cmpDecimal(a, b);
        } break;
      case "wicked": {
          // TODO Complex format that I didn't look up yet.
        } break;
      case "wishbone": {
          // Ends with ";MS"
          score = this.reprMs(src.replace(/^.*;/, ""));
          cmp = (a, b) => this.cmpTime(a, b);
        } break;
      case "xrm": {
          // Semicolon-delimited fields, look for "ot=MM:SS.mmm"
          score = this.sanitizeTime(src.match(/ot=([^;]*);/)?.[1]);
          cmp = (a, b) => this.cmpTime(a, b);
        } break;
      case "younap": {
          // "SCORE;MM:SS.mmm;MM:SS.mmm" first time is Any%, second is 100%. Ach, I really wish we could score them separately. But let's use score.
          score = this.sanitizeDecimal(src.split(";")[0]);
          cmp = (a, b) => this.cmpDecimal(a, b);
        } break;
      case "zennoniwa": {
          // "MM:SS.mmm;NUMBER;NUMBER", not sure what those other two things are.
          score = this.sanitizeTime(src.split(";")[0]);
          cmp = (a, b) => this.cmpTime(a, b);
        } break;
      default: {
          console.log(`ScoreboardService: Unknown score format ${JSON.stringify(fmt)}`);
        } break;
    }
    if (score) {
      let action;
      if (cmp(score, prev) < 0) {
        action = "hiscore";
        this.scores[file] = score;
      } else {
        action = "pity";
      }
      this.broadcast({ action, file, score });
    }
  }
  
  formatTime(hour, min, sec, ms) {
    let dst = "";
    if (hour > 0) {
      dst += hour.toString(); // don't pad
      dst += ":";
    }
    // Always emit minutes. Single digit if there's no hours and it's under ten. (which is very likely).
    if (hour || (min >= 10)) {
      dst += min.toString().padStart(2, "0");
    } else {
      dst += min.toString();
    }
    dst += ":";
    // Seconds and milliseconds pad unconditionally.
    dst += sec.toString().padStart(2, "0");
    dst += ".";
    dst += ms.toString().padStart(3, "0");
    return dst;
  }
  
  reprMs(src) {
    let ms = +src;
    if (isNaN(ms)) return null;
    if (ms < 0) ms = 0;
    let sec = Math.floor(ms / 1000); ms %= 1000;
    let min = Math.floor(sec / 60); sec %= 60;
    let hour = Math.floor(min / 60); min %= 60;
    if (hour > 99) return "99:99:99.999";
    return this.formatTime(hour, min, sec, ms);
  }
  
  sanitizeNopunctTime(src) { // HHMMSSmmm, leading digits may be omitted.
    if (!src) return null;
    if (!src.match(/^\d{4,9}$/)) return null;
    const missing = 9 - src.length;
    src = "000000000".substring(0, missing) + src;
    const hour = +src.substring(0, 2);
    const min = +src.substring(2, 4);
    const sec = +src.substring(4, 6);
    const ms = +src.substring(6, 9);
    if ((min >= 60) || (sec >= 60)) return null;
    return this.formatTime(hour, min, sec, ms);
  }
  
  sanitizeTime(src) {
    if (!src) return null;
    if (src === "99:99:99.999") return src; // Special value which would not technically be legal (too many minutes and seconds).
    const match = src.match(/^(\d{1,2}?):?(\d{1,2}?):?(\d{1,2})\.?(\d{3}?)$/);
    if (!match) return null;
    const hour = +match[1] || 0;
    const min = +match[2] || 0;
    const sec = +match[3] || 0;
    const ms = +match[4] || 0;
    if ((min >= 60) || (sec >= 60)) return null;
    return this.formatTime(hour, min, sec, ms);
  }
  
  cmpTime(a, b) {
    if (!a && !b) return 0;
    if (!a) return 1;
    if (!b) return -1;
    const amatch = a.match(/^(\d{1,2}):?(\d{1,2}?):?(\d{1,2})(\.\d{3})$/);
    const bmatch = b.match(/^(\d{1,2}):?(\d{1,2}?):?(\d{1,2})(\.\d{3})$/);
    if (!amatch && !bmatch) return 0;
    if (!amatch) return 1;
    if (!bmatch) return -1;
    const ahour = +amatch[1] || 0;
    const amin = +amatch[2] || 0;
    const asec = +amatch[3] || 0;
    const ams = +amatch[4] || 0;
    const bhour = +bmatch[1] || 0;
    const bmin = +bmatch[2] || 0;
    const bsec = +bmatch[3] || 0;
    const bms = +bmatch[4] || 0;
    let d;
    if (d = ahour - bhour) return d;
    if (d = amin - bmin) return d;
    if (d = asec - bsec) return d;
    if (d = ams - bms) return d;
    return 0;
  }
  
  sanitizeDecimal(src) {
    const v = +src;
    if (isNaN(v)) return null;
    return v.toString();
  }
  
  cmpDecimal(a, b) {
    if (!a && !b) return 0;
    if (!a) return 1;
    if (!b) return -1;
    a = +a;
    b = +b;
    if (a > b) return -1;
    if (a < b) return 1;
    return 0;
  }
  
  /* State tracking for known games. Probably just Bellacopia.
   ****************************************************************************************/
  
  receiveState(file, host, state) {
    console.log(`ScoreboardService.receiveState ${file} @ ${host}`, state);
    //TODO
  }
}

ScoreboardService.singleton = true;

ScoreboardService.FILE_DISPLAY_NAMES = {
  all52: "All Fifty Two",
  apothecary: "Thirty Seconds Apothecary",
  bellacopia: "Bellacopia Maleficia",
  cherteau: "Cherteau",
  hummfu: "Humm Fu",
  iggle: "Reddin Iggle",
  inversion: "Inversion",
  kabobblin: "Goblin Kabobblin",
  kleptomania: "Kleptomania",
  licensetoilluse: "License to Illuse",
  penance: "Season of Penance",
  presto: "Presto Changeo",
  queenofclocks: "Queen of Clocks",
  samsam: "Sam-Sam",
  upsy: "Upsy-Downsy",
  vexularg: "Vexularg",
  wicked: "Dot's Wicked Garden",
  wishbone: "When You Wish Upon A Bone",
  xrm: "XRM: Extreme Racing Machines",
  younap: "You Could Use A Nap",
  zennoniwa: "Zen Garden",
};

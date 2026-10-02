/* HiscoreUi.js
 * Shows the best score per game.
 */
 
import { Dom } from "./Dom.js";
import { KioskService } from "./KioskService.js";

export class HiscoreUi {
  static getDependencies() {
    return [HTMLElement, Dom, KioskService];
  }
  constructor(element, dom, kioskService) {
    this.element = element;
    this.dom = dom;
    this.kioskService = kioskService;
    
    this.buildUi();
    
    this.kioskListener = this.kioskService.listen(e => this.onKioskEvent(e));
    this.kioskService.iterate(e => this.onKioskEvent({ action: "add", score: e }));
  }
  
  onRemoveFromDom() {
    this.kioskService.unlisten(this.kioskListener);
  }
  
  buildUi() {
    this.element.innerHTML = "";
  }
  
  onKioskEvent(event) {
    //console.log(`HiscoreUi.onKioskEvent`, event);//TODO
    /* XXX TEMPORARY
     * Show each incoming event in its own row at the top.
     * Remove after so many are collected.
     * This isn't the real thing; still just validating the flow.
     */
    const rows = Array.from(this.element.querySelectorAll(".row"));
    while (rows.length >= 5) {
      rows[rows.length-1].remove();
      rows.splice(rows.length-1, 1);
    }
    const row = this.dom.spawn(null, "DIV", ["row"]);
    this.dom.spawn(row, "IMG", ["thumbnail"], { src: `./thumb/${event.score.file}.png` });
    this.dom.spawn(row, "DIV", ["title"], event.score.file);
    this.dom.spawn(row, "DIV", ["host"], event.score.host);
    this.dom.spawn(row, "DIV", ["body"], JSON.stringify(event.score.body));
    this.element.insertBefore(row, rows[0]);
  }
}

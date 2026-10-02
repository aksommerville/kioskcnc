/* RootUi.js
 * Top of our view hierarchy.
 */
 
import { Dom } from "./Dom.js";
import { HiscoreUi } from "./HiscoreUi.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.hiscoreUi = null;
    
    this.element.addEventListener("click", () => this.element.requestFullscreen());
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "DIV", ["headline"], "GDEX 2026 Leaderboard");
    this.hiscoreUi = this.dom.spawnController(this.element, HiscoreUi);
  }
}

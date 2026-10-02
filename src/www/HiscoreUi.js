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
  }
  
  onRemoveFromDom() {
    this.kioskService.unlisten(this.kioskListener);
  }
  
  buildUi() {
    this.element.innerHTML = "";
  }
  
  onKioskEvent(event) {
    console.log(`HiscoreUi.onKioskEvent`, event);//TODO
  }
}

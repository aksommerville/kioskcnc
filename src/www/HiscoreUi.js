/* HiscoreUi.js
 * Shows the best score per game.
 */
 
import { Dom } from "./Dom.js";
import { ScoreboardService } from "./ScoreboardService.js";

export class HiscoreUi {
  static getDependencies() {
    return [HTMLElement, Dom, ScoreboardService];
  }
  constructor(element, dom, scoreboardService) {
    this.element = element;
    this.dom = dom;
    this.scoreboardService = scoreboardService;
    
    // We're going to process maybe hundreds of events the moment we connect the hose.
    // For the first few seconds of life, treat them only as state, don't highlight anything or trigger eventy stuff.
    this.disableNotifications = Date.now() + 5000;
    
    this.buildUi();
    
    this.scoreboardListener = this.scoreboardService.listen(e => this.onScoreboardEvent(e));
  }
  
  onRemoveFromDom() {
    this.scoreboardService.unlisten(this.scoreboardListener);
  }
  
  /* UI.
   *************************************************************************/
  
  buildUi() {
    this.element.innerHTML = "";
    for (const file of Object.keys(this.scoreboardService.scores)) {
      this.createOrPopulateCard(file, this.scoreboardService.scores[file]);
    }
  }
  
  createOrPopulateCard(file, score) {
    let card = this.element.querySelector(`.card[data-file='${file}']`);
    if (card) {
      card.querySelector(".score").innerText = score;
    } else {
      card = this.dom.spawn(this.element, "DIV", ["card"], {
        "data-file": file,
        "on-animationend": e => { card.classList.remove("blinkPity", "blinkHiscore"); },
      });
      this.dom.spawn(card, "IMG", ["thumb"], { src: `./thumb/${file}.png` });
      const text = this.dom.spawn(card, "DIV", ["text"]);
      this.dom.spawn(text, "DIV", ["title"], ScoreboardService.FILE_DISPLAY_NAMES[file] || file);
      this.dom.spawn(text, "DIV", ["scoreAligner"],
        this.dom.spawn(null, "DIV", ["score"], score)
      );
    }
    return card;
  }
  
  /* Scoreboard events.
   **************************************************************************/
   
  setHiscore(file, score) {
    const card = this.createOrPopulateCard(file, score);
    if (this.disableNotifications) {
      if (Date.now() < this.disableNotifications) return;
      this.disableNotifications = null;
    }
    card.classList.add("blinkHiscore");
  }
  
  showPity(file, score) {
    // This exists and it should come up sometimes but usually not: If your score was less than your local record, the save file won't be updated so we never see it here.
    if (this.disableNotifications) {
      if (Date.now() < this.disableNotifications) return;
      this.disableNotifications = null;
    }
    card.classList.add("blinkPity");
  }
  
  onScoreboardEvent(event) {
    switch (event.action) {
      case "hiscore": return this.setHiscore(event.file, event.score);
      case "pity": return this.showPity(event.file, event.score);
      default: console.log(`HiscoreUi.onScoreboardEvent`, event); //TODO
      /*
      case "item":
      case "rootdevil":
      case "treestory":
      case "zoo":
      /**/
    }
  }
}

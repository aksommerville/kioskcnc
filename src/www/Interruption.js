/* Interruption.js
 * Big modal that tells the world about some event.
 */
 
import { Dom } from "./Dom.js";

export class Interruption {
  static getDependencies() {
    return [HTMLElement, Dom, Window];
  }
  constructor(element, dom, window) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    
    this.pgm = "";
    this.arg = null;
    this.cb_complete = null;
  }
  
  setup(pgm, arg, cb) {
    this.pgm = pgm;
    this.arg = arg;
    this.cb_complete = cb;
    this.removalTime = 10000;
    
    switch (pgm) {
      case "allrootdevils": this.setup_allrootdevils(); break;
      case "rootdevil": this.setup_rootdevil(); break;
      case "item": this.setup_item(); break;
      case "election": this.setup_election(); break;
      case "fish": this.setup_fish(); break;
    }
    
    this.window.setTimeout(() => {
      this.cb_complete?.();
      this.element.remove();
    }, this.removalTime);
  }
  
  /* allrootdevils: Dot just won the main quest!
   */
   
  setup_allrootdevils() {
    //TODO
    //this.dom.spawn(this.element, "DIV", "Dot strangled the last root devil!");
  }
  
  /* rootdevil: Strangled one root devil, presumably not the last.
   * (arg) is its location, for display if we want.
   */
   
  setup_rootdevil() {
    this.dom.spawn(this.element, "IMG", ["deadRootDevil"], { src: "./news/deadrootdevil.png" });
    this.removalTime = 6000;
  }
  
  /* item: Acquired something.
   * (arg) is its name.
   */
   
  setup_item() {
    this.element.classList.add("darkish");
    this.dom.spawn(this.element, "DIV", ["itemText"], "Fortune smiles upon thee, Dot,");
    this.dom.spawn(this.element, "IMG", { src: "./news/oohtreasure.png" });
    this.dom.spawn(this.element, "DIV", ["itemText"], `for thou hast found the ${this.arg}.`);
    this.removalTime = 6000;
  }
  
  /* election: Started, won, or gained an endorsement.
   * We can have some fun with this one...
   */
   
  setup_election() {
    switch (this.arg) {
      case 1: { // started
          const blotter = this.dom.spawn(this.element, "DIV", ["voteDotBlotter"]);
          this.dom.spawn(blotter, "DIV", ["bigText"], "Vote Dot for Mayor!");
          this.dom.spawn(blotter, "IMG", ["voteforme"], { src: "./news/voteforme.png" });
          this.window.setTimeout(() => {
            this.dom.spawn(blotter, "DIV", ["speech"], "I'm Dot Vine and I approved this message.");
          }, 2000);
        } break;
      case 2: { // won
          this.dom.spawn(this.element, "IMG", ["newspaper"], { src: "./news/newspaper.png" });
          this.removalTime = 4500;
        } break;
      case 3: { // endorsement
          const banner = this.dom.spawn(this.element, "DIV", ["newsBanner"]);
          const r = Math.random();
          let msg = (
            (r < 0.250) ? "BREAKING NEWS: DOT PULLS AHEAD IN THE POLLS" :
            (r < 0.500) ? "NEWS FLASH: ELECTION TURNING TOWARD DOT" :
            (r < 0.750) ? "NEWS ALERT: DARK HORSE CANDIDATE LEADING IN MAYORAL RACE" :
            "THIS JUST IN: DOT WINS ANOTHER ENDORSEMENT"
          );
          this.dom.spawn(banner, "DIV", ["newsBannerText"], msg);
          this.removalTime = 8000;
        } break;
    }
  }
  
  /* fish: (arg) is "green", "blue", or "red".
   */
   
  setup_fish() {
    const img = this.dom.spawn(this.element, "IMG", ["caughtfish"], { src: `./news/${this.arg}fish.png` });
    this.removalTime = 5000;
  }
}

import { Injector } from "./Injector.js";
import { Dom } from "./Dom.js";
import { RootUi } from "./RootUi.js";

window.addEventListener("load", () => {
  const injector = new Injector(window);
  const dom = injector.instantiate(Dom);
  const rootUi = dom.spawnController(document.body, RootUi);
});

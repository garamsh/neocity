"use strict";

const selection = document.querySelector("#game-selection");
const cards = document.querySelectorAll(".game-card");
const categories = document.querySelectorAll(".catalog-filters [data-category]");
const catalogStatus = document.querySelector("#catalog-status");
const toolbar = document.querySelector(".detail-toolbar");
const detailCategory = document.querySelector("#detail-category");
const comparison = document.querySelector("#comparison");
const groups = document.querySelectorAll("#concepts, #conveyors, #appendix");
const proposals = document.querySelectorAll(".concept, .conveyor-card, .earlier-concept");
const dialog = document.querySelector("#art-dialog");
const dialogImage = document.querySelector("#dialog-image");
const dialogTitle = document.querySelector("#dialog-title");
const originalLink = document.querySelector("#original-link");
let selectedId = null;

function showCatalog(category) {
  let visible = 0;
  cards.forEach((card) => {
    card.hidden = category !== "all" && card.dataset.category !== category;
    if (!card.hidden) visible += 1;
  });
  categories.forEach((link) => {
    if (link.dataset.category === category) link.setAttribute("aria-current", "true");
    else link.removeAttribute("aria-current");
  });
  selection.hidden = false;
  catalogStatus.textContent = `${visible}개 게임 후보를 표시합니다.`;
}

function showView() {
  const previousView = document.body.dataset.view;
  const target = document.getElementById(location.hash.slice(1));
  selection.hidden = true;
  comparison.hidden = true;
  toolbar.hidden = true;
  groups.forEach((group) => { group.hidden = true; });
  proposals.forEach((proposal) => { proposal.hidden = false; });
  document.body.dataset.view = "catalog";

  if (target && target.matches(".concept, .conveyor-card, .earlier-concept")) {
    selectedId = target.id;
    const group = target.closest("#concepts, #conveyors, #appendix");
    group.hidden = false;
    group.querySelectorAll(".concept, .conveyor-card, .earlier-concept").forEach((proposal) => {
      proposal.hidden = proposal !== target;
    });
    toolbar.hidden = false;
    document.body.dataset.view = "detail";
    const heading = target.querySelector("h2, h3");
    detailCategory.textContent = `AI 제안 · ${heading.textContent.trim().replace(/\s+/g, " ")}`;
    heading.tabIndex = -1;
    heading.focus({ preventScroll: true });
  } else if (target === comparison) {
    comparison.hidden = false;
    toolbar.hidden = false;
    detailCategory.textContent = "직업 4개 비교";
    document.body.dataset.view = "comparison";
  } else {
    const category = target && target.id === "concepts" ? "occupation" :
      target && target.id === "conveyors" ? "conveyor" :
      target && target.id === "appendix" ? "earlier" : "all";
    showCatalog(category);
    if (previousView === "detail" && selectedId) {
      const card = [...cards].find((candidate) => candidate.hash === `#${selectedId}` && !candidate.hidden);
      if (card) {
        card.focus({ preventScroll: true });
        card.scrollIntoView({ block: "center" });
        return;
      }
    }
  }
  window.scrollTo(0, 0);
}

document.body.classList.add("enhanced");
window.addEventListener("hashchange", showView);
showView();

if (typeof dialog.showModal === "function") {
  document.querySelectorAll(".art-link").forEach((link) => {
    link.addEventListener("click", (event) => {
      if (event.ctrlKey || event.metaKey || event.shiftKey || event.altKey) return;
      event.preventDefault();
      dialogImage.src = link.href;
      dialogImage.alt = link.querySelector("img").alt;
      dialogTitle.textContent = link.dataset.art;
      originalLink.href = link.href;
      dialog.showModal();
    });
  });
  document.querySelector("#close-dialog").addEventListener("click", () => dialog.close());
  dialog.addEventListener("click", (event) => {
    if (event.target !== dialog) return;
    const bounds = dialog.getBoundingClientRect();
    if (event.clientX < bounds.left || event.clientX > bounds.right ||
        event.clientY < bounds.top || event.clientY > bounds.bottom) dialog.close();
  });
}

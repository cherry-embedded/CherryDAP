import { defineUserConfig } from "vuepress";

import theme from "./theme.js";

export default defineUserConfig({
  base: "/",

  locales: {
    "/en/": {
      lang: "en-US",
      title: "CherryDAP",
      description: "CherryDAP Documentation",
    },
    "/": {
      lang: "zh-CN",
      title: "CherryDAP",
      description: "CherryDAP 文档",
    },
  },

  theme,

  // Enable it with pwa
  // shouldPrefetch: false,
});

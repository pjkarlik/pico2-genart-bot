import { defineConfig } from "vite";
import { resolve } from "path";

export default defineConfig(() => ({
    base: "/",
    build: {
        minify: true,
        cssMinify: true,

        rollupOptions: {
            input: {
                index: resolve(__dirname, "index.html"),
            },
        },
    },

    css: {
        preprocessorOptions: {
            scss: {
                includePaths: ["node_modules"],
                additionalData: `
                    @import 'modern-normalize/modern-normalize.css';
                `,
            },
        },
    },
}));



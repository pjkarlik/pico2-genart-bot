import { defineConfig } from "vite";
import { resolve } from "path";

export default defineConfig(({ command }) => ({
    base: command === "build" ? "/big-sprite" : "/",
    build: {
        minify: true,
        cssMinify: true,
        rollupOptions: {
            input: {
                index: resolve(__dirname, "index.html"),
            },
        },
    },
    server: {
        proxy: {
            '/api': 'http://localhost:8080',
            '/images': 'http://localhost:8080'
        }
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


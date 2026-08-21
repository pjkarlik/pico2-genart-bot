export class Gallery {

    private container: HTMLElement;
    private pagination: HTMLElement;

    private images: string[] = [];
    private currentPage = 1;

    private readonly imagesPerPage = 24;

    constructor(
        gallerySelector: string,
        paginationSelector: string
    ) {
        const gallery = document.querySelector(gallerySelector);
        const pagination = document.querySelector(paginationSelector);

        if (!gallery) {
            throw new Error(`Gallery element "${gallerySelector}" not found`);
        }

        if (!pagination) {
            throw new Error(`Pagination element "${paginationSelector}" not found`);
        }

        this.container = gallery as HTMLElement;
        this.pagination = pagination as HTMLElement;

        const backdrop = document.querySelector('.image-modal__backdrop');

        backdrop?.addEventListener('click', () => {
            this.closeModal();
        });

        document.addEventListener('keydown', (event) => {

            if (event.key === 'Escape') {
                this.closeModal();
            }

        });
    }

    async load() {
        try {
            const response = await fetch('/images.json');

            if (!response.ok) {
                throw new Error('Failed to fetch image list');
            }

            const data = await response.json();

            this.images = data.images;

            this.render();

        } catch (error) {
            console.error('Gallery error:', error);

            this.container.innerHTML = `
                <p>Unable to load images.</p>
            `;
        }
    }

    render() {

        const start = (this.currentPage - 1) * this.imagesPerPage;
        const end = start + this.imagesPerPage;

        const pageImages = this.images.slice(start, end);

        this.container.innerHTML = pageImages
            .map(filename => `
                <button class="gallery-item" data-image="${filename}">
                    <img
                        src="/images/${filename}"
                        alt="${filename}"
                        loading="lazy"
                        width="120"
                        height="120"
                    >
                </div>
            `)
            .join('');

        this.container
            .querySelectorAll<HTMLButtonElement>('.gallery-item')
            .forEach(button => {

                button.addEventListener('click', () => {

                    const filename = button.dataset.image;

                    if (filename) {
                        this.openModal(filename);
                    }
                });

            });
        this.renderPagination();
    }

    renderPagination() {

        const totalPages = Math.ceil(
            this.images.length / this.imagesPerPage
        );

        if (totalPages <= 1) {
            this.pagination.innerHTML = '';
            return;
        }

        let html = '';

        // Previous
        html += `
            <button
                class="pagination-button"
                data-page="${this.currentPage - 1}"
                ${this.currentPage === 1 ? 'disabled' : ''}
            >
                &lt;
            </button>
        `;

        // Page numbers
        for (let page = 1; page <= totalPages; page++) {

            html += `
                <button
                    class="pagination-button ${page === this.currentPage ? 'active' : ''}"
                    data-page="${page}"
                >
                    ${page}
                </button>
            `;
        }

        // Next
        html += `
            <button
                class="pagination-button"
                data-page="${this.currentPage + 1}"
                ${this.currentPage === totalPages ? 'disabled' : ''}
            >
                &gt;
            </button>
        `;

        this.pagination.innerHTML = html;

        this.pagination
            .querySelectorAll<HTMLButtonElement>('[data-page]')
            .forEach(button => {

                button.addEventListener('click', () => {

                    const page = Number(button.dataset.page);

                    this.goToPage(page);
                });
            });
    }

    goToPage(page: number) {

        const totalPages = Math.ceil(
            this.images.length / this.imagesPerPage
        );

        if (page < 1 || page > totalPages) {
            return;
        }

        this.currentPage = page;

        this.render();

        window.scrollTo({
            top: 0,
            behavior: 'smooth'
        });
    }

    private openModal(filename: string) {

        const modal = document.querySelector<HTMLElement>('#image-modal');
        modal.style.backgroundImage = `url("/images/${filename}")`;

        modal.classList.add('is-open');
        modal.setAttribute('aria-hidden', 'false');

        document.body.classList.add('modal-open');
    }

    private closeModal() {

        const modal = document.querySelector<HTMLElement>('#image-modal');

        if (!modal) {
            return;
        }

        modal.classList.remove('is-open');
        modal.setAttribute('aria-hidden', 'true');

        document.body.classList.remove('modal-open');
    }
}
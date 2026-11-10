import './styles.scss';
import { Gallery } from './gallery';


window.addEventListener('DOMContentLoaded', () => {

    const gallery = new Gallery(
        '#gallery',
        '#pagination'
    );

    gallery.load();
});
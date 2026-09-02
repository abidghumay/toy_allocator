/**
 * Toy Allocator Visualizer Frontend Application
 * Interactively renders heap memory blocks, transitions, and step-by-step playback.
 */

class AllocatorVisualizer {
    constructor() {
        this.events = [];
        this.currentIndex = 0;
        this.isPlaying = false;
        this.playTimer = null;
        this.stepDelay = 800; // ms
        this.selectedBlock = null;

        this.initDom();
        this.bindEvents();
        this.loadEvents();
    }

    initDom() {
        // Stats
        this.statUsed = document.getElementById('stat-used');
        this.statBlocks = document.getElementById('stat-blocks');
        this.statAllocated = document.getElementById('stat-allocated');
        this.statFree = document.getElementById('stat-free');

        // Banner
        this.actionBanner = document.getElementById('action-banner');
        this.bannerStep = document.getElementById('banner-step');
        this.bannerEvent = document.getElementById('banner-event');
        this.bannerCall = document.getElementById('banner-call');
        this.bannerDesc = document.getElementById('banner-desc');

        // Heap stage
        this.heapRangeLabel = document.getElementById('heap-range-label');
        this.heapStrip = document.getElementById('heap-strip');
        this.bumpUsedBar = document.getElementById('bump-used-bar');
        this.bumpRemainingLabel = document.getElementById('bump-remaining-label');

        // Inspector
        this.inspectorStatusBadge = document.getElementById('inspector-status-badge');
        this.inspectorEmpty = document.getElementById('inspector-empty');
        this.inspectorDetails = document.getElementById('inspector-details');
        this.inspHeaderAddr = document.getElementById('insp-header-addr');
        this.inspPayloadAddr = document.getElementById('insp-payload-addr');
        this.inspOffset = document.getElementById('insp-offset');
        this.inspPayloadSize = document.getElementById('insp-payload-size');
        this.inspTotalSize = document.getElementById('insp-total-size');
        this.inspStatus = document.getElementById('insp-status');
        this.inspPrevSize = document.getElementById('insp-prev-size');
        this.inspPrevAlloc = document.getElementById('insp-prev-alloc');
        this.archPayloadText = document.getElementById('arch-payload-text');

        // Timeline
        this.timelineList = document.getElementById('timeline-list');
        this.eventCountBadge = document.getElementById('event-count-badge');

        // Controls
        this.btnPlay = document.getElementById('btn-play');
        this.playIcon = document.getElementById('play-icon');
        this.btnPrev = document.getElementById('btn-prev');
        this.btnNext = document.getElementById('btn-next');
        this.btnFirst = document.getElementById('btn-first');
        this.btnLast = document.getElementById('btn-last');
        this.scrubber = document.getElementById('scrubber');
        this.scrubCurrent = document.getElementById('scrub-current');
        this.scrubTotal = document.getElementById('scrub-total');
        this.btnRunDemo = document.getElementById('btn-run-demo');
        this.toast = document.getElementById('toast');
    }

    bindEvents() {
        this.btnPlay.addEventListener('click', () => this.togglePlay());
        this.btnPrev.addEventListener('click', () => this.step(-1));
        this.btnNext.addEventListener('click', () => this.step(1));
        this.btnFirst.addEventListener('click', () => this.jumpTo(0));
        this.btnLast.addEventListener('click', () => this.jumpTo(this.events.length - 1));

        this.scrubber.addEventListener('input', (e) => {
            this.pause();
            this.jumpTo(parseInt(e.target.value, 10));
        });

        // Speed buttons
        document.querySelectorAll('.btn-speed').forEach(btn => {
            btn.addEventListener('click', (e) => {
                document.querySelectorAll('.btn-speed').forEach(b => b.classList.remove('active'));
                btn.classList.add('active');
                this.stepDelay = parseInt(btn.dataset.speed, 10);
                if (this.isPlaying) {
                    this.pause();
                    this.play();
                }
            });
        });

        this.btnRunDemo.addEventListener('click', () => this.runDemo());

        // Keyboard navigation
        window.addEventListener('keydown', (e) => {
            if (e.target.tagName === 'INPUT') return;
            if (e.code === 'Space') {
                e.preventDefault();
                this.togglePlay();
            } else if (e.code === 'ArrowLeft') {
                e.preventDefault();
                this.pause();
                this.step(-1);
            } else if (e.code === 'ArrowRight') {
                e.preventDefault();
                this.pause();
                this.step(1);
            } else if (e.code === 'Home') {
                e.preventDefault();
                this.jumpTo(0);
            } else if (e.code === 'End') {
                e.preventDefault();
                this.jumpTo(this.events.length - 1);
            }
        });
    }

    async loadEvents() {
        try {
            const res = await fetch('/api/events');
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            this.events = data.events || [];

            if (this.events.length === 0) {
                this.showToast('No events logged yet. Click "Run Demo" to start!');
                return;
            }

            this.scrubber.max = this.events.length - 1;
            this.scrubTotal.textContent = this.events.length;
            this.eventCountBadge.textContent = `${this.events.length} events`;

            this.buildTimeline();
            this.jumpTo(0);
            this.showToast(`Loaded ${this.events.length} allocator trace events.`);
        } catch (err) {
            console.error('Failed to load events:', err);
            this.showToast('Could not fetch events. Is server.py running?');
        }
    }

    async runDemo() {
        this.pause();
        this.btnRunDemo.disabled = true;
        this.btnRunDemo.innerHTML = '<span class="btn-icon">⏳</span> Running...';
        this.showToast('Rebuilding and executing demo program...');

        try {
            const res = await fetch('/api/run-demo', { method: 'POST' });
            const data = await res.json();
            if (data.success && data.events && data.events.length > 0) {
                this.events = data.events;
                this.scrubber.max = this.events.length - 1;
                this.scrubTotal.textContent = this.events.length;
                this.eventCountBadge.textContent = `${this.events.length} events`;
                this.buildTimeline();
                this.jumpTo(0);
                this.showToast('Demo executed successfully! Replaying trace...');
                setTimeout(() => this.play(), 400);
            } else {
                this.showToast('Demo failed: ' + (data.output || 'Unknown error'));
            }
        } catch (err) {
            console.error('Demo execution error:', err);
            this.showToast('Error running demo: ' + err.message);
        } finally {
            this.btnRunDemo.disabled = false;
            this.btnRunDemo.innerHTML = '<span class="btn-icon">⚡</span> Run Demo';
        }
    }

    buildTimeline() {
        this.timelineList.innerHTML = '';
        this.events.forEach((ev, idx) => {
            const item = document.createElement('div');
            item.className = `timeline-item`;
            item.dataset.index = idx;

            const stepSpan = document.createElement('span');
            stepSpan.className = 'tl-step';
            stepSpan.textContent = `#${ev.seq}`;

            const evSpan = document.createElement('span');
            evSpan.className = `tl-event ${ev.event}`;
            evSpan.textContent = ev.event.replace('COALESCE_', 'COAL_');

            const descSpan = document.createElement('span');
            descSpan.className = 'tl-desc';
            descSpan.textContent = ev.call ? `${ev.call}: ${ev.desc}` : ev.desc;

            item.appendChild(stepSpan);
            item.appendChild(evSpan);
            item.appendChild(descSpan);

            item.addEventListener('click', () => {
                this.pause();
                this.jumpTo(idx);
            });

            this.timelineList.appendChild(item);
        });
    }

    togglePlay() {
        if (this.isPlaying) {
            this.pause();
        } else {
            this.play();
        }
    }

    play() {
        if (this.currentIndex >= this.events.length - 1) {
            this.currentIndex = 0;
        }
        this.isPlaying = true;
        this.playIcon.textContent = '⏸';
        this.btnPlay.classList.add('playing');

        this.playTimer = setInterval(() => {
            if (this.currentIndex < this.events.length - 1) {
                this.step(1);
            } else {
                this.pause();
            }
        }, this.stepDelay);
    }

    pause() {
        this.isPlaying = false;
        this.playIcon.textContent = '▶';
        this.btnPlay.classList.remove('playing');
        if (this.playTimer) {
            clearInterval(this.playTimer);
            this.playTimer = null;
        }
    }

    step(direction) {
        const next = this.currentIndex + direction;
        if (next >= 0 && next < this.events.length) {
            this.jumpTo(next);
        }
    }

    jumpTo(index) {
        if (this.events.length === 0) return;
        this.currentIndex = Math.max(0, Math.min(index, this.events.length - 1));
        this.scrubber.value = this.currentIndex;
        this.scrubCurrent.textContent = this.currentIndex + 1;

        const currentEvent = this.events[this.currentIndex];
        this.renderEvent(currentEvent);
        this.highlightTimelineItem(this.currentIndex);
    }

    highlightTimelineItem(index) {
        const items = this.timelineList.querySelectorAll('.timeline-item');
        items.forEach((item, idx) => {
            if (idx === index) {
                item.classList.add('active');
                item.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
            } else {
                item.classList.remove('active');
            }
        });
    }

    renderEvent(ev) {
        if (!ev) return;

        // Banner details
        this.bannerStep.textContent = `Step ${ev.seq} / ${this.events.length}`;
        this.bannerEvent.textContent = ev.event;
        this.bannerCall.textContent = ev.call || 'Internal heap action';
        this.bannerDesc.textContent = ev.desc;

        // Classify banner color by event type
        this.actionBanner.className = 'action-banner';
        if (ev.event === 'ALLOC') this.actionBanner.classList.add('event-ALLOC');
        else if (ev.event === 'SPLIT') this.actionBanner.classList.add('event-SPLIT');
        else if (ev.event.startsWith('COALESCE')) this.actionBanner.classList.add('event-COALESCE');
        else if (ev.event === 'FREE') this.actionBanner.classList.add('event-FREE');

        // Address range
        const startAddr = ev.heap_start || '0x0';
        const movingAddr = ev.movingptr || '0x0';
        this.heapRangeLabel.textContent = `${startAddr} (heap_start) → ${movingAddr} (movingptr)`;

        // Stats calculation
        let allocatedBytes = 0;
        let freeBytes = 0;
        (ev.blocks || []).forEach(b => {
            if (b.allocated) allocatedBytes += b.payload_size;
            else freeBytes += b.payload_size;
        });

        this.statUsed.textContent = `${ev.used_bytes || 0} B`;
        this.statBlocks.textContent = (ev.blocks || []).length;
        this.statAllocated.textContent = `${allocatedBytes} B`;
        this.statFree.textContent = `${freeBytes} B`;

        // Bump tracker
        const capacity = ev.heap_capacity || (1024 * 1024);
        const usedPercent = Math.min(100, Math.max(0.5, ((ev.used_bytes || 0) / capacity) * 100));
        this.bumpUsedBar.style.width = `${Math.max(1, usedPercent * 10)}%`; // Visual exaggeration for 1MB heap
        const remainingBytes = capacity - (ev.used_bytes || 0);
        this.bumpRemainingLabel.textContent = `${(remainingBytes / 1024).toFixed(1)} KB uncommitted in 1MB heap`;

        // Render Blocks in Strip
        this.renderBlocks(ev);
    }

    renderBlocks(ev) {
        const blocks = ev.blocks || [];
        this.heapStrip.innerHTML = '';

        if (blocks.length === 0) {
            const emptyNotice = document.createElement('div');
            emptyNotice.className = 'empty-state';
            emptyNotice.style.padding = '20px';
            emptyNotice.textContent = 'Heap is initialized; no blocks allocated yet.';
            this.heapStrip.appendChild(emptyNotice);
            this.clearInspector();
            return;
        }

        let targetFound = null;

        blocks.forEach(block => {
            const el = document.createElement('div');
            el.className = 'heap-block';
            el.dataset.addr = block.addr;

            // State class
            if (block.allocated) {
                el.classList.add('state-allocated');
            } else {
                el.classList.add('state-free');
            }

            // Highlight class
            if (block.highlight === 'just_split' || block.highlight === 'just_split_leftover') {
                el.classList.add('highlight-split');
                targetFound = block;
            } else if (block.highlight === 'just_coalesced') {
                el.classList.add('highlight-coalesce');
                targetFound = block;
            } else if (block.highlight === 'just_freed') {
                el.classList.add('highlight-freed');
                targetFound = block;
            } else if (block.highlight === 'just_allocated') {
                el.classList.add('highlight-alloc');
                targetFound = block;
            }

            // Proportional sizing with minimum readability
            el.style.flex = `${block.total_size} 1 65px`;

            // Short address snippet (last 4 chars)
            const shortAddr = block.addr.length > 6 ? block.addr.slice(-4) : block.addr;

            // Content
            el.innerHTML = `
                <div class="block-top">
                    <span class="block-tag">${block.allocated ? 'Alloc' : 'Free'}</span>
                    <span class="block-size">${block.payload_size}B</span>
                </div>
                <div class="block-bottom">
                    <span class="block-addr">..${shortAddr}</span>
                    <span class="block-hdr-indicator">H:16B</span>
                </div>
            `;

            // Click to inspect
            el.addEventListener('click', () => {
                this.selectBlock(block, el);
            });

            this.heapStrip.appendChild(el);
        });

        // Unallocated remainder block
        const unallocated = document.createElement('div');
        unallocated.className = 'heap-unallocated';
        unallocated.innerHTML = `
            <span><strong>Unallocated Heap</strong></span>
            <span>(Bump Space)</span>
        `;
        this.heapStrip.appendChild(unallocated);

        // Update inspector: if user selected a block, keep it; otherwise inspect the active block
        if (targetFound) {
            this.selectBlock(targetFound);
        } else if (blocks.length > 0) {
            this.selectBlock(blocks[0]);
        }
    }

    selectBlock(block, blockEl = null) {
        this.selectedBlock = block;

        // Visual selection indicator
        this.heapStrip.querySelectorAll('.heap-block').forEach(b => {
            if (b.dataset.addr === block.addr) {
                b.classList.add('selected');
            } else {
                b.classList.remove('selected');
            }
        });

        // Update Inspector panel
        this.inspectorEmpty.classList.add('hidden');
        this.inspectorDetails.classList.remove('hidden');

        this.inspectorStatusBadge.textContent = block.allocated ? 'ALLOCATED' : 'FREE';
        this.inspectorStatusBadge.style.color = block.allocated ? 'var(--color-alloc)' : 'var(--color-free-border)';

        this.inspHeaderAddr.textContent = block.addr;
        this.inspPayloadAddr.textContent = block.payload_addr;
        this.inspOffset.textContent = `+${block.offset} bytes from heap_start`;
        this.inspPayloadSize.textContent = `${block.payload_size} bytes (aligned to 16B)`;
        this.inspTotalSize.textContent = `${block.total_size} bytes (${block.payload_size}B payload + 16B header)`;
        this.inspStatus.textContent = block.allocated ? 'Allocated (Size bit 0 = 1)' : 'Free (Size bit 0 = 0)';
        this.inspPrevSize.textContent = `${block.prev_size} bytes`;
        this.inspPrevAlloc.textContent = block.prev_allocated ? 'Allocated (1)' : 'Free (0)';

        this.archPayloadText.textContent = `Payload: ${block.payload_size} Bytes (${block.allocated ? 'Active' : 'Unused'})`;
    }

    clearInspector() {
        this.inspectorEmpty.classList.remove('hidden');
        this.inspectorDetails.classList.add('hidden');
        this.inspectorStatusBadge.textContent = 'None';
    }

    showToast(message) {
        this.toast.textContent = message;
        this.toast.classList.remove('hidden');
        setTimeout(() => {
            this.toast.classList.add('hidden');
        }, 3200);
    }
}

// Instantiate on load
document.addEventListener('DOMContentLoaded', () => {
    window.visualizer = new AllocatorVisualizer();
});

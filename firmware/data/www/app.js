// DMX-Open PWA — v0.1.0

(function () {
'use strict';

// ── State ───────────────────────────────────────────────────

const state = {
    ch: new Uint8Array(512),
    master: 255,
    player: { loaded: false, playing: false, paused: false, show: '', pos: 0, duration: 0, loop: false },
    shows: [],
    connected: false,
    bank: 0,
    bankSize: 16,
    editorShow: null
};

// ── WebSocket ───────────────────────────────────────────────

let ws = null;
let wsRetry = null;

function wsConnect() {
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
    ws = new WebSocket(proto + '//' + location.host + '/ws');

    ws.onopen = () => {
        state.connected = true;
        updateConnDot();
        ws.send(JSON.stringify({ cmd: 'getState' }));
    };

    ws.onclose = () => {
        state.connected = false;
        updateConnDot();
        ws = null;
        wsRetry = setTimeout(wsConnect, 2000);
    };

    ws.onerror = () => ws.close();

    ws.onmessage = (e) => {
        let msg;
        try { msg = JSON.parse(e.data); } catch { return; }

        if (msg.type === 'dmx') {
            if (msg.ch) for (let i = 0; i < 512; i++) state.ch[i] = msg.ch[i] || 0;
            if (msg.master !== undefined) state.master = msg.master;
            if (currentView === 'live') updateFaderValues();
        }
        else if (msg.type === 'player') {
            Object.assign(state.player, msg);
            updateTransport();
        }
    };
}

function wsSend(obj) {
    if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(obj));
}

function updateConnDot() {
    const dot = document.getElementById('conn-dot');
    if (dot) dot.className = 'dot ' + (state.connected ? 'online' : 'offline');
}

// ── Router ──────────────────────────────────────────────────

let currentView = '';

function route() {
    const hash = location.hash.replace('#', '') || 'live';
    currentView = hash;

    document.querySelectorAll('.tab').forEach(t => {
        t.classList.toggle('active', t.getAttribute('href') === '#' + hash);
    });

    const app = document.getElementById('app');
    switch (hash) {
        case 'live':     app.innerHTML = renderLive(); initLive(); break;
        case 'shows':    app.innerHTML = renderShows(); loadShows(); break;
        case 'editor':   app.innerHTML = renderEditor(); initEditor(); break;
        case 'settings': app.innerHTML = renderSettings(); initSettings(); break;
        default:         app.innerHTML = renderLive(); initLive();
    }
}

// ── Transport controls ──────────────────────────────────────

function initTransport() {
    document.getElementById('btn-play').onclick = () => wsSend({ cmd: 'play' });
    document.getElementById('btn-pause').onclick = () => wsSend({ cmd: 'pause' });
    document.getElementById('btn-stop').onclick = () => wsSend({ cmd: 'stop' });
    document.getElementById('btn-blackout').onclick = () => {
        wsSend({ cmd: 'blackout' });
        state.ch.fill(0);
        if (currentView === 'live') updateFaderValues();
    };
}

function updateTransport() {
    const p = state.player;
    document.getElementById('btn-play').classList.toggle('active', p.playing);
    document.getElementById('btn-pause').classList.toggle('active', p.paused);

    const info = document.getElementById('player-info');
    if (p.playing || p.paused) {
        const pos = formatTime(p.pos);
        const dur = formatTime(p.duration);
        info.textContent = (p.show || '?') + ' ' + pos + '/' + dur;
    } else if (p.loaded) {
        info.textContent = p.show || 'loaded';
    } else {
        info.textContent = '';
    }
}

function formatTime(ms) {
    const s = Math.floor(ms / 1000);
    const m = Math.floor(s / 60);
    return m + ':' + String(s % 60).padStart(2, '0');
}

// ── Live view ───────────────────────────────────────────────

function renderLive() {
    const banks = Math.ceil(512 / state.bankSize);
    let bankBtns = '';
    for (let b = 0; b < banks; b++) {
        const from = b * state.bankSize + 1;
        const to = Math.min((b + 1) * state.bankSize, 512);
        bankBtns += `<button class="bank-btn${b === state.bank ? ' active' : ''}" data-bank="${b}">${from}-${to}</button>`;
    }

    let faders = '';
    const start = state.bank * state.bankSize;
    for (let i = 0; i < state.bankSize; i++) {
        const ch = start + i;
        if (ch >= 512) break;
        const val = state.ch[ch];
        const pct = (val / 255 * 100).toFixed(1);
        faders += `
        <div class="fader" data-ch="${ch}">
            <div class="fader-num">${ch + 1}</div>
            <div class="fader-track" data-ch="${ch}">
                <div class="fader-fill" style="height:${pct}%"></div>
                <div class="fader-thumb" style="bottom:calc(${pct}% - 6px)"></div>
            </div>
            <div class="fader-val">${val}</div>
        </div>`;
    }

    return `
    <div class="master-row">
        <span class="master-label">Master</span>
        <input type="range" class="master-slider" id="master-slider" min="0" max="255" value="${state.master}">
        <span class="master-val" id="master-val">${state.master}</span>
    </div>
    <div class="bank-nav" id="bank-nav">${bankBtns}</div>
    <div class="fader-bank" id="fader-bank">${faders}</div>`;
}

function initLive() {
    // Master slider
    const ms = document.getElementById('master-slider');
    if (ms) {
        ms.oninput = () => {
            state.master = parseInt(ms.value);
            document.getElementById('master-val').textContent = state.master;
            wsSend({ cmd: 'master', value: state.master });
        };
    }

    // Bank switching
    document.getElementById('bank-nav').addEventListener('click', (e) => {
        if (e.target.dataset.bank !== undefined) {
            state.bank = parseInt(e.target.dataset.bank);
            document.getElementById('app').innerHTML = renderLive();
            initLive();
        }
    });

    // Fader touch/mouse interaction
    const bank = document.getElementById('fader-bank');
    let activeFader = null;

    function setFaderValue(track, y) {
        const rect = track.getBoundingClientRect();
        let pct = 1 - (y - rect.top) / rect.height;
        pct = Math.max(0, Math.min(1, pct));
        const val = Math.round(pct * 255);
        const ch = parseInt(track.dataset.ch);

        state.ch[ch] = val;
        const fader = track.closest('.fader');
        fader.querySelector('.fader-fill').style.height = (pct * 100) + '%';
        fader.querySelector('.fader-thumb').style.bottom = 'calc(' + (pct * 100) + '% - 6px)';
        fader.querySelector('.fader-val').textContent = val;

        wsSend({ cmd: 'set', ch: { [ch + 1]: val } });
    }

    bank.addEventListener('pointerdown', (e) => {
        const track = e.target.closest('.fader-track');
        if (!track) return;
        e.preventDefault();
        track.setPointerCapture(e.pointerId);
        activeFader = track;
        setFaderValue(track, e.clientY);
    });

    bank.addEventListener('pointermove', (e) => {
        if (!activeFader) return;
        e.preventDefault();
        setFaderValue(activeFader, e.clientY);
    });

    bank.addEventListener('pointerup', () => { activeFader = null; });
    bank.addEventListener('pointercancel', () => { activeFader = null; });
}

function updateFaderValues() {
    const faders = document.querySelectorAll('#fader-bank .fader');
    faders.forEach(f => {
        const ch = parseInt(f.dataset.ch);
        const val = state.ch[ch];
        const pct = (val / 255 * 100).toFixed(1);
        f.querySelector('.fader-fill').style.height = pct + '%';
        f.querySelector('.fader-thumb').style.bottom = 'calc(' + pct + '% - 6px)';
        f.querySelector('.fader-val').textContent = val;
    });
    const ms = document.getElementById('master-slider');
    if (ms && document.activeElement !== ms) {
        ms.value = state.master;
        document.getElementById('master-val').textContent = state.master;
    }
}

// ── Shows view ──────────────────────────────────────────────

function renderShows() {
    return `
    <div id="show-list" class="show-list">
        <div class="empty-state">Loading...</div>
    </div>`;
}

async function loadShows() {
    try {
        const r = await fetch('/api/shows');
        state.shows = await r.json();
    } catch { state.shows = []; }

    const el = document.getElementById('show-list');
    if (!state.shows.length) {
        el.innerHTML = '<div class="empty-state">No shows on device.<br>Use the Editor to create one.</div>';
        return;
    }

    el.innerHTML = state.shows.map(s => `
        <div class="show-item" data-show="${esc(s.name)}">
            <span class="show-name">${esc(s.name)}</span>
            <span class="show-size">${(s.size / 1024).toFixed(1)}k</span>
            <div class="show-actions">
                <button class="show-btn play" data-action="play" data-show="${esc(s.name)}">Play</button>
                <button class="show-btn" data-action="edit" data-show="${esc(s.name)}">Edit</button>
                <button class="show-btn del" data-action="delete" data-show="${esc(s.name)}">Del</button>
            </div>
        </div>`).join('');

    el.addEventListener('click', async (e) => {
        const btn = e.target.closest('[data-action]');
        if (!btn) return;
        const action = btn.dataset.action;
        const name = btn.dataset.show;

        if (action === 'play') {
            wsSend({ cmd: 'play', show: name });
        } else if (action === 'edit') {
            await loadShowForEdit(name);
            location.hash = '#editor';
        } else if (action === 'delete') {
            if (confirm('Delete "' + name + '"?')) {
                await fetch('/api/shows?name=' + encodeURIComponent(name), { method: 'DELETE' });
                loadShows();
            }
        }
    });
}

async function loadShowForEdit(name) {
    try {
        const r = await fetch('/api/shows');
        const shows = await r.json();
        const show = shows.find(s => s.name === name);
        if (!show) return;

        // Fetch the actual show file content via a workaround:
        // We'll load it into the player and re-construct, or fetch raw
        // For now, store the name and let editor re-fetch
        state.editorShow = { name, steps: [], loop: false };

        // Try to get show data by loading from the device's filesystem
        // The API doesn't expose raw files, so we'll use a fetch to the shows dir
        const raw = await fetch('/shows/' + name + '.json');
        if (raw.ok) {
            state.editorShow = await raw.json();
        }
    } catch (e) {
        state.editorShow = null;
    }
}

// ── Editor view ─────────────────────────────────────────────

function renderEditor() {
    const show = state.editorShow || { name: '', steps: [], loop: false };

    let stepsHtml = '';
    if (show.steps && show.steps.length) {
        stepsHtml = show.steps.map((s, i) => renderStep(s, i)).join('');
    } else {
        stepsHtml = '<div class="empty-state">No steps yet. Add one to begin.</div>';
    }

    return `
    <div class="editor-toolbar">
        <button class="ed-btn primary" id="ed-new">New Show</button>
        <button class="ed-btn" id="ed-add-step">+ Step</button>
        <button class="ed-btn primary" id="ed-save">Save to Device</button>
        <button class="ed-btn" id="ed-preview">Preview</button>
    </div>
    <div class="ed-meta">
        <div class="ed-field">
            <label>Show Name</label>
            <input type="text" id="ed-name" value="${esc(show.name || '')}" placeholder="my-show">
        </div>
        <div class="ed-field">
            <label>Loop</label>
            <select id="ed-loop">
                <option value="false" ${!show.loop ? 'selected' : ''}>No</option>
                <option value="true" ${show.loop ? 'selected' : ''}>Yes</option>
            </select>
        </div>
    </div>
    <div class="step-list" id="step-list">${stepsHtml}</div>`;
}

function renderStep(step, idx) {
    const chTags = (step.ch || []).map(cv =>
        `<span class="ch-tag"><span class="ch-num">${cv[0]}:</span>${cv[1]}</span>`
    ).join('');

    return `
    <div class="step-item" data-idx="${idx}">
        <div class="step-header">
            <input type="number" class="ed-field" style="width:80px" value="${step.t || 0}"
                   data-field="t" data-idx="${idx}" placeholder="ms">
            <span class="step-fade">fade:
                <input type="number" style="width:60px;background:var(--bg-input);border:1px solid #ffffff15;color:var(--text);border-radius:4px;padding:2px 6px;font-size:12px"
                       value="${step.fade || 0}" data-field="fade" data-idx="${idx}"> ms
            </span>
            <button class="step-remove" data-remove="${idx}">&times;</button>
        </div>
        <div class="step-channels">${chTags || '<span style="color:var(--text-dim);font-size:12px">No channels — click to add</span>'}</div>
        <button class="ed-btn" style="margin-top:8px;font-size:11px" data-edit-ch="${idx}">Edit Channels</button>
    </div>`;
}

function initEditor() {
    if (!state.editorShow) {
        state.editorShow = { name: '', steps: [], loop: false };
    }
    const show = state.editorShow;

    document.getElementById('ed-new').onclick = () => {
        state.editorShow = { name: '', steps: [], loop: false };
        document.getElementById('app').innerHTML = renderEditor();
        initEditor();
    };

    document.getElementById('ed-add-step').onclick = () => {
        const lastT = show.steps.length ? show.steps[show.steps.length - 1].t + 2000 : 0;
        show.steps.push({ t: lastT, fade: 1000, ch: [] });
        refreshStepList();
    };

    document.getElementById('ed-save').onclick = async () => {
        const name = document.getElementById('ed-name').value.trim();
        if (!name) { toast('Enter a show name'); return; }
        show.name = name;
        show.loop = document.getElementById('ed-loop').value === 'true';

        try {
            const r = await fetch('/api/shows', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(show)
            });
            if (r.ok) toast('Saved "' + name + '"');
            else toast('Save failed');
        } catch { toast('Save failed — offline?'); }
    };

    document.getElementById('ed-preview').onclick = () => {
        const name = document.getElementById('ed-name').value.trim();
        if (!name) { toast('Save first'); return; }
        wsSend({ cmd: 'play', show: name });
    };

    bindStepListEvents();
}

function refreshStepList() {
    const show = state.editorShow;
    const el = document.getElementById('step-list');
    if (show.steps.length) {
        el.innerHTML = show.steps.map((s, i) => renderStep(s, i)).join('');
    } else {
        el.innerHTML = '<div class="empty-state">No steps yet.</div>';
    }
    bindStepListEvents();
}

function bindStepListEvents() {
    const el = document.getElementById('step-list');
    const show = state.editorShow;

    el.querySelectorAll('[data-field]').forEach(input => {
        input.onchange = () => {
            const idx = parseInt(input.dataset.idx);
            const field = input.dataset.field;
            show.steps[idx][field] = parseInt(input.value) || 0;
        };
    });

    el.querySelectorAll('[data-remove]').forEach(btn => {
        btn.onclick = () => {
            show.steps.splice(parseInt(btn.dataset.remove), 1);
            refreshStepList();
        };
    });

    el.querySelectorAll('[data-edit-ch]').forEach(btn => {
        btn.onclick = () => openChannelEditor(parseInt(btn.dataset.editCh));
    });
}

function openChannelEditor(stepIdx) {
    const step = state.editorShow.steps[stepIdx];
    const channels = step.ch || [];

    let rows = channels.map((cv, i) =>
        `<div class="setting-row">
            <input type="number" min="1" max="512" value="${cv[0]}" data-ci="${i}" data-f="ch" style="flex:1">
            <input type="number" min="0" max="255" value="${cv[1]}" data-ci="${i}" data-f="val" style="flex:1">
            <button class="show-btn del" data-rm-ch="${i}">&times;</button>
        </div>`
    ).join('');

    const modal = document.createElement('div');
    modal.className = 'modal-overlay';
    modal.innerHTML = `
    <div class="modal">
        <h2>Step ${stepIdx + 1} — Channels</h2>
        <div id="ch-rows">${rows || '<div style="color:var(--text-dim)">No channels</div>'}</div>
        <div style="margin-top:12px">
            <button class="ed-btn" id="ch-add">+ Channel</button>
            <button class="ed-btn" id="ch-snap">Snapshot Live</button>
        </div>
        <div class="modal-actions">
            <button class="btn" id="ch-cancel">Cancel</button>
            <button class="btn btn-primary" id="ch-ok">OK</button>
        </div>
    </div>`;

    document.body.appendChild(modal);

    const workingCh = channels.map(cv => [...cv]);

    modal.querySelector('#ch-add').onclick = () => {
        workingCh.push([1, 0]);
        refreshChRows();
    };

    modal.querySelector('#ch-snap').onclick = () => {
        // Snapshot current live DMX values for non-zero channels
        workingCh.length = 0;
        for (let i = 0; i < 512; i++) {
            if (state.ch[i] > 0) workingCh.push([i + 1, state.ch[i]]);
        }
        refreshChRows();
    };

    function refreshChRows() {
        const el = modal.querySelector('#ch-rows');
        el.innerHTML = workingCh.map((cv, i) =>
            `<div class="setting-row">
                <input type="number" min="1" max="512" value="${cv[0]}" data-ci="${i}" data-f="ch" style="flex:1">
                <input type="number" min="0" max="255" value="${cv[1]}" data-ci="${i}" data-f="val" style="flex:1">
                <button class="show-btn del" data-rm-ch="${i}">&times;</button>
            </div>`
        ).join('') || '<div style="color:var(--text-dim)">No channels</div>';

        el.querySelectorAll('input').forEach(inp => {
            inp.onchange = () => {
                const ci = parseInt(inp.dataset.ci);
                if (inp.dataset.f === 'ch') workingCh[ci][0] = parseInt(inp.value) || 1;
                else workingCh[ci][1] = parseInt(inp.value) || 0;
            };
        });

        el.querySelectorAll('[data-rm-ch]').forEach(btn => {
            btn.onclick = () => {
                workingCh.splice(parseInt(btn.dataset.rmCh), 1);
                refreshChRows();
            };
        });
    }
    refreshChRows();

    modal.querySelector('#ch-cancel').onclick = () => modal.remove();
    modal.querySelector('#ch-ok').onclick = () => {
        step.ch = workingCh.filter(cv => cv[0] >= 1 && cv[0] <= 512);
        modal.remove();
        refreshStepList();
    };
    modal.addEventListener('click', (e) => { if (e.target === modal) modal.remove(); });
}

// ── Settings view ───────────────────────────────────────────

function renderSettings() {
    return `
    <div class="settings-section">
        <h3>WiFi</h3>
        <div class="setting-row">
            <label>Network SSID</label>
            <input type="text" id="set-ssid" placeholder="your-wifi">
        </div>
        <div class="setting-row">
            <label>Password</label>
            <input type="password" id="set-pass" placeholder="password">
        </div>
        <button class="btn btn-primary" id="set-wifi-save">Connect</button>
    </div>
    <div class="settings-section">
        <h3>Playback</h3>
        <div class="setting-row">
            <label>Autoplay show</label>
            <select id="set-autoplay">
                <option value="">None</option>
            </select>
        </div>
        <button class="btn btn-primary" id="set-playback-save">Save</button>
    </div>
    <div class="settings-section">
        <h3>Device Info</h3>
        <div id="device-info" style="font-size:13px;color:var(--text-dim)">Loading...</div>
    </div>`;
}

async function initSettings() {
    // Load current config
    try {
        const r = await fetch('/api/config');
        const cfg = await r.json();
        if (cfg.wifi_ssid) document.getElementById('set-ssid').value = cfg.wifi_ssid;

        // Load show list for autoplay dropdown
        const sr = await fetch('/api/shows');
        const shows = await sr.json();
        const sel = document.getElementById('set-autoplay');
        shows.forEach(s => {
            const opt = document.createElement('option');
            opt.value = s.name;
            opt.textContent = s.name;
            if (cfg.autoplay === s.name) opt.selected = true;
            sel.appendChild(opt);
        });
    } catch {}

    // Load device status
    try {
        const r = await fetch('/api/status');
        const s = await r.json();
        document.getElementById('device-info').innerHTML = `
            Version: ${s.version}<br>
            AP IP: ${s.ap_ip}<br>
            STA IP: ${s.sta_connected ? s.sta_ip : 'not connected'}<br>
            ${s.sta_connected ? 'Network: ' + s.sta_ssid + '<br>' : ''}
            Free heap: ${(s.heap / 1024).toFixed(0)} KB<br>
            Uptime: ${Math.floor(s.uptime / 60)}m ${s.uptime % 60}s`;
    } catch {
        document.getElementById('device-info').textContent = 'Could not reach device';
    }

    document.getElementById('set-wifi-save').onclick = async () => {
        const ssid = document.getElementById('set-ssid').value;
        const pass = document.getElementById('set-pass').value;
        try {
            await fetch('/api/config', {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ wifi_ssid: ssid, wifi_pass: pass })
            });
            toast('WiFi config saved — connecting...');
        } catch { toast('Failed to save'); }
    };

    document.getElementById('set-playback-save').onclick = async () => {
        const autoplay = document.getElementById('set-autoplay').value;
        try {
            await fetch('/api/config', {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ autoplay })
            });
            toast('Playback config saved');
        } catch { toast('Failed to save'); }
    };
}

// ── Toast ───────────────────────────────────────────────────

function toast(msg) {
    const el = document.createElement('div');
    el.className = 'toast';
    el.textContent = msg;
    document.body.appendChild(el);
    setTimeout(() => el.remove(), 2500);
}

// ── Util ────────────────────────────────────────────────────

function esc(s) {
    const d = document.createElement('div');
    d.textContent = s;
    return d.innerHTML;
}

// ── Boot ────────────────────────────────────────────────────

window.addEventListener('hashchange', route);
window.addEventListener('DOMContentLoaded', () => {
    initTransport();
    wsConnect();
    route();

    if ('serviceWorker' in navigator) {
        navigator.serviceWorker.register('/sw.js').catch(() => {});
    }
});

})();

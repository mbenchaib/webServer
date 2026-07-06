const STORAGE_KEYS = {
  theme: 'webserv.theme',
  history: 'webserv.history',
};

const DEFAULT_HEADERS = [
  ['Accept', '*/*'],
  ['Content-Type', 'application/json'],
];

const state = {
  history: loadHistory(),
  lastResult: null,
};

function qs(selector, parent) {
  return (parent || document).querySelector(selector);
}

function qsa(selector, parent) {
  return Array.prototype.slice.call((parent || document).querySelectorAll(selector));
}

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

function formatBytes(bytes) {
  if (!Number.isFinite(bytes) || bytes < 0) {
    return '0 B';
  }

  var units = ['B', 'KB', 'MB', 'GB'];
  var value = bytes;
  var index = 0;

  while (value >= 1024 && index < units.length - 1) {
    value /= 1024;
    index += 1;
  }

  return value.toFixed(index === 0 ? 0 : 1) + ' ' + units[index];
}

function formatDuration(ms) {
  return Math.max(0, Math.round(ms)) + ' ms';
}

function formatTime(timestamp) {
  return new Date(timestamp).toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
}

function loadHistory() {
  try {
    return JSON.parse(localStorage.getItem(STORAGE_KEYS.history) || '[]');
  } catch (_) {
    return [];
  }
}

function saveHistory() {
  localStorage.setItem(STORAGE_KEYS.history, JSON.stringify(state.history.slice(0, 60)));
}

function pushHistory(entry) {
  state.history.unshift(entry);
  state.history = state.history.slice(0, 60);
  saveHistory();
}

function loadTheme() {
  return localStorage.getItem(STORAGE_KEYS.theme) || 'dark';
}

function applyTheme(theme) {
  document.documentElement.dataset.theme = theme;
  localStorage.setItem(STORAGE_KEYS.theme, theme);

  var toggle = qs('[data-theme-toggle]');
  if (toggle) {
    var nextTheme = theme === 'dark' ? 'light' : 'dark';
    toggle.setAttribute('aria-label', 'Switch to ' + nextTheme + ' theme');
    toggle.title = 'Switch to ' + nextTheme + ' theme';
    toggle.textContent = theme === 'dark' ? '☾' : '☀';
  }
}

function toggleTheme() {
  applyTheme(loadTheme() === 'dark' ? 'light' : 'dark');
}

function initTheme() {
  applyTheme(loadTheme());
  var toggle = qs('[data-theme-toggle]');
  if (toggle) {
    toggle.addEventListener('click', toggleTheme);
  }
}

function bindMeta() {
  var mapping = {
    host: location.hostname || 'localhost',
    port: location.port || '80',
    origin: location.origin,
    route: location.pathname,
    root: '/www1',
  };

  qsa('[data-bind]').forEach(function (node) {
    var key = node.dataset.bind;
    if (mapping[key] !== undefined) {
      node.textContent = mapping[key];
    }
  });
}

function bindActiveNav() {
  var current = document.body.dataset.page;
  qsa('[data-page-link]').forEach(function (link) {
    link.classList.toggle('is-active', link.dataset.pageLink === current);
  });
}

function initRefreshButton() {
  var refresh = qs('[data-refresh-page]');
  if (refresh) {
    refresh.addEventListener('click', function () {
      window.location.reload();
    });
  }
}

function setText(node, value) {
  if (node) {
    node.textContent = value;
  }
}

function updateOverview(result) {
  var chip = qs('[data-server-chip]');
  var stateNode = qs('[data-server-state]');
  var response = qs('[data-server-response]');
  var time = qs('[data-server-time]');
  var length = qs('[data-server-length]');
  var count = qs('[data-request-count]');
  var activeRoute = qs('[data-active-route]');
  var statusBadge = qs('[data-dashboard-response-badge]');
  var summary = qs('[data-dashboard-summary]');

  if (stateNode) {
    stateNode.textContent = result && result.ok ? 'Online' : 'Offline';
    stateNode.classList.toggle('is-online', !!(result && result.ok));
    stateNode.classList.toggle('is-offline', !(result && result.ok));
  }

  if (chip) {
    chip.classList.toggle('status-chip--good', !!(result && result.ok));
    chip.classList.toggle('status-chip--bad', !(result && result.ok));
  }

  if (result) {
    setText(response, (result.status + ' ' + result.statusText).trim());
    setText(time, formatDuration(result.duration));
    setText(length, formatBytes(result.contentLength));
    setText(count, String(state.history.length));
    setText(activeRoute, result.requestUrl || '/');

    if (statusBadge) {
      statusBadge.textContent = formatDuration(result.duration) + ' • ' + formatBytes(result.contentLength);
    }
    if (summary) {
      summary.textContent = result.ok ? 'Server responded successfully.' : 'The last request returned an error or failed to reach the server.';
    }
  }
}

function parseContentLength(result) {
  if (result.contentLengthHeader !== null && result.contentLengthHeader !== undefined && result.contentLengthHeader !== '') {
    var parsed = Number(result.contentLengthHeader);
    if (!Number.isNaN(parsed)) {
      return parsed;
    }
  }
  return new Blob([result.bodyText || '']).size;
}

function isTextualContentType(contentType) {
  return /^(text\/|application\/(json|xml|javascript|xhtml\+xml))/i.test(contentType) || /\b(svg|json|xml|javascript)\b/i.test(contentType);
}

async function readBody(response, contentType) {
    if (isTextualContentType(contentType)) {
        return response.text();
    }

    if (/^image\//i.test(contentType)) {
        var blob = await response.blob();
        return URL.createObjectURL(blob);
    }

    var buffer = await response.arrayBuffer();
    return '[binary body omitted: ' + buffer.byteLength + ' bytes]';
}

function buildUrl(pathname) {
  return new URL(pathname || '/', location.href).href;
}

async function performRequest(descriptor) {
  var method = (descriptor.method || 'GET').toUpperCase();
  var requestUrl = descriptor.url || '/';
  var started = performance.now();
  var headers = new Headers(descriptor.headers || {});
  var options = {
    method: method,
    headers: headers,
    credentials: 'same-origin',
  };

  if (descriptor.cache) {
    options.cache = descriptor.cache;
  }

  if (descriptor.body !== undefined && descriptor.body !== null && method !== 'GET' && method !== 'HEAD') {
    if (descriptor.body instanceof FormData) {
      options.body = descriptor.body;
      headers.delete('Content-Type');
    } else {
      options.body = descriptor.body;
    }
  }

  try {
    var response = await fetch(buildUrl(requestUrl), options);
    var contentType = response.headers.get('content-type') || '';
    var headersList = [];
    response.headers.forEach(function (value, key) {
      headersList.push([key, value]);
    });

    var bodyText = await readBody(response.clone(), contentType).catch(function () {
      return '';
    });
    var result = {
      ok: response.ok,
      status: response.status,
      statusText: response.statusText,
      requestUrl: requestUrl,
      resolvedUrl: response.url,
      method: method,
      headers: headersList,
      bodyText: bodyText,
      contentType: contentType,
      contentLengthHeader: response.headers.get('content-length'),
      contentLength: parseContentLength({ bodyText: bodyText, contentLengthHeader: response.headers.get('content-length') }),
      duration: performance.now() - started,
      timestamp: Date.now(),
      label: descriptor.label || method + ' ' + requestUrl,
    };

    pushHistory({
      method: result.method,
      url: result.requestUrl,
      status: (result.status + ' ' + result.statusText).trim(),
      time: formatTime(result.timestamp),
      responseTime: formatDuration(result.duration),
      contentLength: formatBytes(result.contentLength),
    });

    state.lastResult = result;
    updateOverview(result);
    return result;
  } catch (error) {
    var failed = {
      ok: false,
      status: 0,
      statusText: 'Network Error',
      requestUrl: requestUrl,
      resolvedUrl: requestUrl,
      method: method,
      headers: [],
      bodyText: error && error.message ? error.message : String(error),
      contentType: '',
      contentLengthHeader: null,
      contentLength: 0,
      duration: performance.now() - started,
      timestamp: Date.now(),
      label: descriptor.label || method + ' ' + requestUrl,
    };

    pushHistory({
      method: failed.method,
      url: failed.requestUrl,
      status: failed.statusText,
      time: formatTime(failed.timestamp),
      responseTime: formatDuration(failed.duration),
      contentLength: '0 B',
    });

    state.lastResult = failed;
    updateOverview(failed);
    return failed;
  }
}

function renderHistoryTable(root, limit) {
  if (!root) {
    return;
  }

  var rows = state.history.slice(0, limit || 10);
  if (!rows.length) {
    root.innerHTML = '<div class="empty-state">No requests yet. Use any tester to create live history.</div>';
    return;
  }

  var body = rows.map(function (row) {
    return '<div class="table-row">' +
      '<span><span class="badge">' + escapeHtml(row.method) + '</span></span>' +
      '<span>' + escapeHtml(row.url) + '</span>' +
      '<span>' + escapeHtml(row.status) + '</span>' +
      '<span>' + escapeHtml(row.responseTime) + '</span>' +
      '<span>' + escapeHtml(row.time) + '</span>' +
    '</div>';
  }).join('');

  root.innerHTML = '<div class="table-row"><span>Method</span><span>URL</span><span>Status</span><span>Time</span><span>When</span></div>' + body;
}

function renderResponse(result, root) {
  if (!root || !result) {
    return;
  }

  setText(qs('[data-response-status]', root), (result.status + ' ' + result.statusText).trim());
  setText(qs('[data-response-duration]', root), formatDuration(result.duration));
  setText(qs('[data-response-length]', root), formatBytes(result.contentLength));
  setText(qs('[data-response-type]', root), result.contentType || 'unknown');
  setText(qs('[data-response-url]', root), result.requestUrl);
  setText(qs('[data-response-label]', root), result.label);
  setText(qs('[data-response-message]', root), result.ok ? 'Request completed successfully.' : 'Request failed or returned an error status.');

  var headersList = qs('[data-response-headers]', root);
  if (headersList) {
    headersList.innerHTML = result.headers.length ? result.headers.map(function (pair) {
      return '<div class="table-row"><span>' + escapeHtml(pair[0]) + '</span><span>' + escapeHtml(pair[1]) + '</span><span></span><span></span><span></span></div>';
    }).join('') : '<div class="empty-state">No response headers returned.</div>';
  }

  var body = qs('[data-response-body]', root);
  if (body) {
    body.textContent = result.bodyText || '[empty body]';
  }

var preview = qs('[data-response-preview]', root);

if (preview) {

    if (/^image\//i.test(result.contentType)) {

        preview.srcdoc =
            '<html><body style="margin:0;background:#111;display:flex;justify-content:center;align-items:center;height:100vh;">' +
            '<img src="' + result.bodyText + '" style="max-width:100%;max-height:100%;object-fit:contain;">' +
            '</body></html>';

    }
    else if (/text\/html|application\/xhtml\+xml/i.test(result.contentType)) {

        preview.srcdoc = result.bodyText || '<html><body></body></html>';

    }
    else {

        preview.srcdoc =
            '<html><body style="font-family:Inter,Arial,sans-serif;padding:24px;background:#0f1117;color:#e6edf7;">' +
            '<pre style="white-space:pre-wrap;line-height:1.6;">' +
            escapeHtml(result.bodyText || '[preview not available]') +
            '</pre></body></html>';

    }
}
}

function clearResponse(root) {
  if (!root) {
    return;
  }

  setText(qs('[data-response-status]', root), 'Waiting');
  setText(qs('[data-response-duration]', root), '0 ms');
  setText(qs('[data-response-length]', root), '0 B');
  setText(qs('[data-response-type]', root), 'unknown');
  setText(qs('[data-response-url]', root), '—');
  setText(qs('[data-response-label]', root), 'No request yet');
  setText(qs('[data-response-message]', root), 'Send a request to inspect a real server response.');

  var headersList = qs('[data-response-headers]', root);
  if (headersList) {
    headersList.innerHTML = '<div class="empty-state">Send a request to inspect response headers.</div>';
  }

  var body = qs('[data-response-body]', root);
  if (body) {
    body.textContent = 'Response body will appear here.';
  }
// console.log(preview);
  var preview = qs('[data-response-preview]', root);
  if (preview) {
    preview.srcdoc = '<html><body style="font-family:Inter,Arial,sans-serif;padding:24px;background:#0f1117;color:#e6edf7;">Preview will appear here.</body></html>';
  }
}

function addHeaderRow(container, pair) {
  if (!container) {
    return;
  }

  var row = document.createElement('div');
  row.className = 'header-row';
  row.innerHTML = '<input class="input" type="text" placeholder="Header name" value="' + escapeHtml(pair && pair[0] ? pair[0] : '') + '" data-header-name />' +
    '<input class="input" type="text" placeholder="Header value" value="' + escapeHtml(pair && pair[1] ? pair[1] : '') + '" data-header-value />' +
    '<button class="icon-btn" type="button" data-header-remove aria-label="Remove header row">−</button>';

  var remove = qs('[data-header-remove]', row);
  if (remove) {
    remove.addEventListener('click', function () {
      row.remove();
    });
  }

  container.appendChild(row);
}

function initHeaderBuilder() {
  var container = qs('[data-header-rows]');
  var addButton = qs('[data-add-header]');

  if (!container) {
    return;
  }

  if (!qsa('.header-row', container).length) {
    DEFAULT_HEADERS.forEach(function (pair) {
      addHeaderRow(container, pair);
    });
  }

  if (addButton) {
    addButton.addEventListener('click', function () {
      addHeaderRow(container, ['', '']);
    });
  }
}

function readHeaders(container) {
  var headers = {};
  qsa('.header-row', container).forEach(function (row) {
    var name = qs('[data-header-name]', row);
    var value = qs('[data-header-value]', row);
    if (name && name.value.trim()) {
      headers[name.value.trim()] = value ? value.value.trim() : '';
    }
  });
  return headers;
}

function getFormField(form, selector) {
  return qs(selector, form);
}

function getFormValue(form, selector) {
  var field = getFormField(form, selector);
  return field ? field.value : '';
}

async function runPlayground(form) {
  var sendButton = qs('[data-send-request]', form);
  var output = qs('[data-playground-response]');
  var method = getFormValue(form, '[name="method"]');
  var url = getFormValue(form, '[name="url"]').trim();
  var body = getFormValue(form, '[name="body"]');
  var headers = readHeaders(qs('[data-header-rows]', form));

  if (sendButton) {
    sendButton.disabled = true;
    sendButton.textContent = 'Sending...';
  }

  var result = await performRequest({
    method: method,
    url: url,
    body: body,
    headers: headers,
    label: 'Playground ' + method + ' ' + url,
  });

  renderResponse(result, output);
  renderHistoryTable(qs('[data-history-table]'), 10);

  if (sendButton) {
    sendButton.disabled = false;
    sendButton.textContent = 'Send request';
  }
}

function resetPlayground(form) {
  var container = qs('[data-header-rows]', form);
  var output = qs('[data-playground-response]');
  var hint = qs('[data-playground-hint]');

  if (form) {
    form.reset();
  }

  if (container) {
    container.innerHTML = '';
    DEFAULT_HEADERS.forEach(function (pair) {
      addHeaderRow(container, pair);
    });
  }

  if (hint) {
    hint.textContent = 'Ready to send a new request.';
  }

  clearResponse(output);
}

function applyQueryDefaults() {
  var params = new URLSearchParams(location.search);
  var method = params.get('method');
  var url = params.get('url');
  var body = params.get('body');

  var methodField = qs('[name="method"]');
  var urlField = qs('[name="url"]');
  var bodyField = qs('[name="body"]');

  if (method && methodField) {
    methodField.value = method.toUpperCase();
  }
  if (url && urlField) {
    urlField.value = url;
  }
  if (body && bodyField) {
    bodyField.value = body;
  }
}

function initPlayground() {
  initHeaderBuilder();
  applyQueryDefaults();
  clearResponse(qs('[data-playground-response]'));
  renderHistoryTable(qs('[data-history-table]'), 10);

  var form = qs('[data-playground-form]');
  if (form) {
    form.addEventListener('submit', function (event) {
      event.preventDefault();
      runPlayground(form);
    });
  }

  var clearButton = qs('[data-clear-body]');
  if (clearButton) {
    clearButton.addEventListener('click', function () {
      var bodyField = qs('[name="body"]');
      if (bodyField) {
        bodyField.value = '';
      }
    });
  }

  var resetButton = qs('[data-reset-form]');
  if (resetButton) {
    resetButton.addEventListener('click', function () {
      resetPlayground(form);
    });
  }
}

function updateHistoryAndResponse(result, root) {
  renderResponse(result, root);
  renderHistoryTable(qs('[data-history-table]'), 10);
}

function buildRequestFromButton(button) {
  var body = button.dataset.body || '';

  if (button.dataset.bodyTemplate === 'invalid-multipart') {
    body = '--broken-boundary\r\nContent-Disposition: form-data; name="file"; filename="probe.txt"\r\nContent-Type: text/plain\r\n\r\nthis body is intentionally malformed';
  }

  if (button.dataset.bodyTemplate === 'large') {
    body = new Array(1025).join('webserv-body-probe-');
  }

  return {
    method: button.dataset.method || 'GET',
    url: button.dataset.url || '/',
    body: body,
    headers: button.dataset.contentType ? { 'Content-Type': button.dataset.contentType } : {},
    label: button.dataset.label || (button.dataset.method || 'GET') + ' ' + (button.dataset.url || '/'),
  };
}

function getRootForAction(trigger) {
  return trigger.closest('[data-dashboard-root]') || qs('[data-dashboard-response]') || qs('[data-playground-response]');
}

async function runQuickButton(button) {
  var action = button.dataset.requestAction || 'fetch';
  var root = getRootForAction(button);

  if (action === 'keepalive') {
    return runKeepAlive(button, root);
  }

  if (action === 'autoindex') {
    return runAutoindex(button, root);
  }

  if (action === 'mime') {
    return runMime(button, root);
  }

  if (action === 'error') {
    return runErrorProbe(button, root);
  }

  if (action === 'upload') {
    return runUpload(button, root);
  }

  return updateHistoryAndResponse(await performRequest(buildRequestFromButton(button)), root);
}

async function runUpload(form, root) {
  var url = getFormValue(form, '[name="upload-url"]') || '/upload';
  var fileInput = getFormField(form, '[name="upload-file"]');
  var note = qs('[data-upload-note]', form);
  var preview = fileInput && fileInput.files && fileInput.files[0] ? fileInput.files[0] : null;

  if (!preview) {
    if (note) {
      note.textContent = 'Choose a file before uploading.';
    }
    return;
  }

  var formData = new FormData();
  formData.append('file', preview, preview.name);

  var result = await performRequest({
    method: 'POST',
    url: url,
    body: formData,
    label: 'UPLOAD ' + url + ' (' + preview.name + ')',
  });

  if (note) {
    note.textContent = result.ok ? 'Upload sent successfully.' : 'Upload finished with a non-success status.';
  }

  updateHistoryAndResponse(result, root);
}

async function runAutoindex(button, root) {
  var form = button.closest('form');
  var path = form ? getFormValue(form, '[name="autoindex-url"]') : button.dataset.url;
  var result = await performRequest({
    method: 'GET',
    url: path || '/images',
    label: 'AUTOINDEX ' + (path || '/images'),
  });

  updateHistoryAndResponse(result, root);

  var list = qs('[data-autoindex-list]', root);
  if (!list) {
    return;
  }

  var entries = [];
  if (/text\/html|application\/xhtml\+xml/i.test(result.contentType)) {
    var parser = new DOMParser();
    var doc = parser.parseFromString(result.bodyText || '', 'text/html');
    qsa('a[href]', doc).forEach(function (anchor) {
      var href = anchor.getAttribute('href') || '';
      var label = anchor.textContent.trim();
      if (href) {
        entries.push({ href: href, label: label || href });
      }
    });
  }

  if (!entries.length) {
    list.innerHTML = '<div class="empty-state">No directory links were detected in the returned HTML.</div>';
    return;
  }

  list.innerHTML = '<div class="log-list">' + entries.map(function (entry) {
    return '<div class="log-entry"><div><strong>' + escapeHtml(entry.label) + '</strong><span>' + escapeHtml(entry.href) + '</span></div><em>directory item</em></div>';
  }).join('') + '</div>';
}

async function runMime(button, root) {
  var result = await performRequest(buildRequestFromButton(button));
  updateHistoryAndResponse(result, root);

  var chip = qs('[data-mime-result]', root);
  if (chip) {
    chip.textContent = result.contentType || 'unknown';
  }
}

async function runErrorProbe(button, root) {
  var result = await performRequest(buildRequestFromButton(button));
  updateHistoryAndResponse(result, root);
}

async function runKeepAlive(button, root) {
  var form = button.closest('form');
  var url = form ? getFormValue(form, '[name="keepalive-url"]') : (button.dataset.url || '/');
  var second = form ? getFormValue(form, '[name="keepalive-second"]') : (button.dataset.secondaryUrl || '/index.html');
  var note = qs('[data-keepalive-note]', root);
  var log = qs('[data-keepalive-log]', root);

  if (note) {
    note.textContent = 'Running two sequential requests through the same browser session. Connection reuse is managed by the browser.';
  }

  var first = await performRequest({ method: 'GET', url: url, label: 'KEEP-ALIVE #1 ' + url });
  var secondResult = await performRequest({ method: 'GET', url: second, label: 'KEEP-ALIVE #2 ' + second });
  updateHistoryAndResponse(secondResult, root);

  if (log) {
    log.innerHTML = '<div class="log-list">' + [first, secondResult].map(function (result, index) {
      return '<div class="log-entry"><div><strong>Request ' + (index + 1) + '</strong><span>' + escapeHtml(result.requestUrl) + '</span></div><em>' + escapeHtml((result.status + ' ' + result.statusText).trim()) + '</em></div>';
    }).join('') + '</div>';
  }

  if (note) {
    note.textContent = first.ok && secondResult.ok ? 'Both sequential requests completed successfully.' : 'At least one request failed; inspect the shared response panel above.';
  }
}

function initDashboardForms() {
  qsa('[data-request-form="post"]').forEach(function (form) {
    form.addEventListener('submit', async function (event) {
      event.preventDefault();

      var url = getFormValue(form, '[name="post-url"]') || '/';
      var body = getFormValue(form, '[name="post-body"]');
      var headers = readHeaders(qs('[data-header-rows]', form));

      if (!headers['Content-Type']) {
        headers['Content-Type'] = 'text/plain; charset=utf-8';
      }

      var result = await performRequest({
        method: 'POST',
        url: url,
        body: body,
        headers: headers,
        label: 'POST ' + url,
      });

      updateHistoryAndResponse(result, qs('[data-dashboard-response]'));
    });
  });

  qsa('[data-request-form="delete"]').forEach(function (form) {
    form.addEventListener('submit', async function (event) {
      event.preventDefault();

      var url = getFormValue(form, '[name="delete-url"]') || '/';
      var result = await performRequest({
        method: 'DELETE',
        url: url,
        label: 'DELETE ' + url,
      });

      updateHistoryAndResponse(result, qs('[data-dashboard-response]'));
    });
  });

  qsa('[data-request-form="upload"]').forEach(function (form) {
    form.addEventListener('submit', async function (event) {
      event.preventDefault();
      await runUpload(form, qs('[data-dashboard-response]'));
    });
  });

  qsa('[data-request-form="mime"]').forEach(function (form) {
    form.addEventListener('submit', async function (event) {
      event.preventDefault();

      var url = getFormValue(form, '[name="mime-url"]') || '/index.html';
      var result = await performRequest({
        method: 'GET',
        url: url,
        label: 'MIME ' + url,
      });

      updateHistoryAndResponse(result, qs('[data-dashboard-response]'));
      var chip = qs('[data-mime-result]', form);
      if (chip) {
        chip.textContent = result.contentType || 'unknown';
      }
    });
  });

  qsa('[data-request-form="autoindex"]').forEach(function (form) {
    form.addEventListener('submit', async function (event) {
      event.preventDefault();
      await runAutoindex(form.querySelector('[data-autoindex-submit]') || form, qs('[data-dashboard-response]'));
    });
  });

  qsa('[data-request-form="keepalive"]').forEach(function (form) {
    form.addEventListener('submit', async function (event) {
      event.preventDefault();
      await runKeepAlive(form.querySelector('[data-keepalive-submit]') || form, qs('[data-dashboard-response]'));
    });
  });
}

function initDashboardButtons() {
  qsa('[data-request-action="fetch"], [data-request-action="error"], [data-request-action="mime"], [data-request-action="keepalive"], [data-request-action="autoindex"], [data-request-action="upload"]').forEach(function (button) {
    button.addEventListener('click', async function (event) {
      event.preventDefault();
      await runQuickButton(button);
      renderHistoryTable(qs('[data-history-table]'), 12);
      setText(qs('[data-request-count]'), String(state.history.length));
    });
  });
}

function initDashboard() {
  renderHistoryTable(qs('[data-history-table]'), 12);
  clearResponse(qs('[data-dashboard-response]'));
  updateOverview(state.lastResult);

  performRequest({
    method: 'GET',
    url: '/',
    label: 'Dashboard bootstrap GET /',
  }).then(function (result) {
    renderResponse(result, qs('[data-dashboard-response]'));
    renderHistoryTable(qs('[data-history-table]'), 12);
  });

  initDashboardForms();
  initDashboardButtons();
}

function initQuickLabelClock() {
  var clock = qs('[data-clock]');
  if (!clock) {
    return;
  }

  var update = function () {
    clock.textContent = new Date().toLocaleTimeString([], {
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
    });
  };

  update();
  window.setInterval(update, 1000);
}

function initDashboardExtras() {
  setText(qs('[data-request-count]'), String(state.history.length));
  setText(qs('[data-active-route]'), '/');
}

function boot() {
  initTheme();
  bindMeta();
  bindActiveNav();
  initRefreshButton();
  initQuickLabelClock();
  initDashboardExtras();

  if (document.body.dataset.page === 'dashboard') {
    initDashboard();
  }

  if (document.body.dataset.page === 'playground') {
    initPlayground();
  }
}

document.addEventListener('DOMContentLoaded', boot);

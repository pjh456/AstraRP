#!/usr/bin/env node
const assert = require('assert');
const fs = require('fs');

function simulateOldPropagation() {
  const adjacency = new Map([
    ['infer_1', ['out_1', 'out_2']],
    ['out_1', ['out_3']],
    ['out_2', ['out_3']],
    ['out_3', ['out_4']],
    ['out_4', []]
  ]);

  const nodePathCount = new Map([['infer_1', 1]]);
  const queue = ['infer_1'];

  while (queue.length > 0) {
    const current = queue.shift();
    const currentCount = nodePathCount.get(current) || 0;
    for (const next of adjacency.get(current) || []) {
      nodePathCount.set(next, (nodePathCount.get(next) || 0) + currentCount);
      queue.push(next);
    }
  }

  return nodePathCount;
}

function simulateNewPropagation() {
  const adjacency = new Map([
    ['infer_1', ['out_1', 'out_2']],
    ['out_1', ['out_3']],
    ['out_2', ['out_3']],
    ['out_3', ['out_4']],
    ['out_4', []]
  ]);

  const nodePathCount = new Map();
  const pending = new Map([['infer_1', 1]]);

  while (pending.size > 0) {
    const [current, delta] = pending.entries().next().value;
    pending.delete(current);
    nodePathCount.set(current, (nodePathCount.get(current) || 0) + delta);

    for (const next of adjacency.get(current) || []) {
      pending.set(next, (pending.get(next) || 0) + delta);
    }
  }

  return nodePathCount;
}

function verifyServerGuard() {
  const source = fs.readFileSync('server.js', 'utf8');
  assert(source.includes('inferenceNodeIds'));
  assert(source.includes('has no upstream input'));
  assert(source.includes('incomingCount'));
}

function main() {
  console.log('[1/2] 验证旧/新传导计数差异...');
  const oldCount = simulateOldPropagation();
  const newCount = simulateNewPropagation();
  console.log('old out_4 =', oldCount.get('out_4'));
  console.log('new out_4 =', newCount.get('out_4'));
  assert.strictEqual(oldCount.get('out_4'), 4);
  assert.strictEqual(newCount.get('out_4'), 2);

  console.log('[2/2] 验证后端存在 infer 无上游保护逻辑...');
  verifyServerGuard();

  console.log('✅ 调试脚本执行通过');
}

main();

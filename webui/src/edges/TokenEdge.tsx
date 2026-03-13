import { BaseEdge, getSmoothStepPath } from '@xyflow/react';
import type { EdgeProps } from '@xyflow/react';

export default function TokenEdge({
  sourceX,
  sourceY,
  targetX,
  targetY,
  sourcePosition,
  targetPosition,
  style,
  markerEnd,
  data
}: EdgeProps) {
  // 使用 getSmoothStepPath 替代 getBezierPath
  const [edgePath] = getSmoothStepPath({
    sourceX,
    sourceY,
    sourcePosition,
    targetX,
    targetY,
    targetPosition,
    borderRadius: 16, // 这里可以自定义拐角的圆角半径，默认通常是 5
  });

  const tokens = (data as { tokens?: string[] } | undefined)?.tokens ?? [];

  return (
    <>
      <BaseEdge path={edgePath} markerEnd={markerEnd} style={style} />

      {tokens.length > 0 && (
        <g>
          <filter id="glow">
            <feGaussianBlur stdDeviation="2.5" result="coloredBlur" />
            <feMerge>
              <feMergeNode in="coloredBlur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
          <circle r="4" className="fill-violet-400" filter="url(#glow)">
            <animateMotion dur="1.5s" repeatCount="indefinite" path={edgePath} rotate="auto" />
          </circle>
        </g>
      )}
    </>
  );
}
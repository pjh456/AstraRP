import { BaseEdge, getBezierPath } from '@xyflow/react';
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
  const [edgePath, labelX, labelY] = getBezierPath({
    sourceX,
    sourceY,
    sourcePosition,
    targetX,
    targetY,
    targetPosition
  });

  const tokens = (data as { tokens?: string[] } | undefined)?.tokens ?? [];
  const latestToken = tokens.length > 0 ? tokens[tokens.length - 1] : '';
  const tailPreview = tokens.slice(-4).join('');

  return (
    <>
      <BaseEdge path={edgePath} markerEnd={markerEnd} style={style} />

      {tokens.length > 0 && (
        <>
          <circle r="3.5" className="fill-violet-400 drop-shadow-[0_0_6px_rgba(168,85,247,0.9)]">
            <animateMotion dur="1.8s" repeatCount="indefinite" path={edgePath} rotate="auto" />
          </circle>

          <g transform={`translate(${labelX}, ${labelY})`}>
            <rect
              x="-56"
              y="-14"
              rx="8"
              ry="8"
              width="112"
              height="28"
              className="fill-slate-900/90 stroke-violet-500/80"
              strokeWidth="1"
            />
            <text
              x="0"
              y="5"
              textAnchor="middle"
              className="fill-violet-200 font-mono text-[10px]"
            >
              {tailPreview || latestToken}
            </text>
          </g>
        </>
      )}
    </>
  );
}

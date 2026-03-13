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
  const [edgePath] = getBezierPath({
    sourceX,
    sourceY,
    sourcePosition,
    targetX,
    targetY,
    targetPosition
  });

  const tokens = (data as { tokens?: string[] } | undefined)?.tokens ?? [];

  return (
    <>
      <BaseEdge path={edgePath} markerEnd={markerEnd} style={style} />

      {tokens.length > 0 && (
        <circle r="3.5" className="fill-violet-400 drop-shadow-[0_0_6px_rgba(168,85,247,0.9)]">
          <animateMotion dur="1.8s" repeatCount="indefinite" path={edgePath} rotate="auto" />
        </circle>
      )}
    </>
  );
}
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

            {tokens.map((t: string, i: number) => (
                <text
                    key={i}
                    className="font-mono text-xs fill-purple-400 font-bold"
                    style={{
                        offsetPath: `path("${edgePath}")`,
                        offsetDistance: `${i * 8}%`,
                        animation: "tokenFlow 2.5s linear",
                        opacity: 0
                    }}
                >
                    {t}
                </text>
            ))}

            <style>
                {`
                @keyframes tokenFlow {
  0% {
    offset-distance: 0%;
    opacity: 1;
  }

  85% {
    opacity: 1;
  }

  100% {
    offset-distance: 100%;
    opacity: 0;
  }
}
            `}
            </style>
        </>
    );
}
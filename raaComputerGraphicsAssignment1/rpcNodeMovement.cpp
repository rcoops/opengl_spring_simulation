﻿#include "rpcNodeMovement.h"

nodePositioning g_eCurrentNodePositioning = none;
nodePositioning g_eSavedPreviousPositioning = none;

void resetNodeForce(raaNode *pNode)
{
	vecInitDVec(pNode->m_vfForce);
}

void calculateSpringForce(raaArc *pArc)
{
	float vfArc[4], vfArcDirection[4], vfForce[4];
	vecInitDVec(vfArc); vecInitDVec(vfArcDirection); vecInitDVec(vfForce);

	vecSub(pArc->m_pNode0->m_afPosition, pArc->m_pNode1->m_afPosition, vfArc); // Calc arc length vector

	float fCurrentArcLength = vecNormalise(vfArc, vfArcDirection); // Calc scalar distance and direction vector
	float fCurrentArcExtension = (fCurrentArcLength - pArc->m_fIdealLen) / pArc->m_fIdealLen; // Calc spring extension
	float fForce = fCurrentArcExtension * pArc->m_fSpringCoef; // Calc spring force

	vecScalarProduct(vfArcDirection, fForce, vfForce);
	vecSub(pArc->m_pNode0->m_vfForce, vfForce, pArc->m_pNode0->m_vfForce);
	vecAdd(pArc->m_pNode1->m_vfForce, vfForce, pArc->m_pNode1->m_vfForce);
}

void calculateNodeMotion(raaNode *pNode)
{
	float vfAcceleration[4], vfDisplacement[4];
	vecInitDVec(vfAcceleration); vecInitDVec(vfDisplacement);

	vecScalarProduct(pNode->m_vfForce, 1 / pNode->m_fMass, vfAcceleration); // a = f/m
						
	for (int i = 0; i < 3; ++i) // s = vt + (at^2)/2
	{
		vfDisplacement[i] = (pNode->m_vfVelocity[i] * csg_fTimeUnit) + vfAcceleration[i] * pow(csg_fTimeUnit, 2) * 0.5f;
	}

	vecAdd(pNode->m_afPosition, vfDisplacement, pNode->m_afPosition); // p' = p + s

	vecCopy(vecScalarProduct(vfDisplacement, 1.0f / csg_fTimeUnit, vfDisplacement), pNode->m_vfVelocity, 3);

	// Apply damping v^''=v'*(1.0-∑_0^1 DampingCoef)
	vecScalarProduct(pNode->m_vfVelocity, csg_fDampeningCoefficient, pNode->m_vfVelocity);
}

void togglePositioning(nodePositioning npPositioning)
{
	g_eCurrentNodePositioning = g_eCurrentNodePositioning == npPositioning ? none : npPositioning; // toggle current positioning on/off
}

void calculateNodeMovement(raaSystem *pSystem)
{
	switch (g_eCurrentNodePositioning)
	{
	case springs:
		visitNodes(pSystem, resetNodeForce);
		visitArcs(pSystem, calculateSpringForce);
		visitNodes(pSystem, calculateNodeMotion);
		break;
	case worldOrder:
		visitNodes(pSystem, moveToWorldOrderPositions);
		break;
	case continent:
		visitNodes(pSystem, moveToContinentPositions);
		break;
	case none:
		break;
	}
}

void pauseMovement()
{
	if (g_eCurrentNodePositioning == none) // go back to original setting
	{
		g_eCurrentNodePositioning = g_eSavedPreviousPositioning;
	}
	else // save original setting and wipe
	{
		g_eSavedPreviousPositioning = g_eCurrentNodePositioning;
		g_eCurrentNodePositioning = none;
	}
}

void moveToSortedOrder(float *afNewPosition, raaNode *pNode)
{
	float vfRoute[4], vfDirection[4];
	vecInitDVec(vfRoute); vecInitDVec(vfDirection);
	vecSub(pNode->m_afPosition, afNewPosition, vfRoute); // Calc route from original to target position
	float fCurrentDistance = vecNormalise(vfRoute, vfDirection); // Calc scalar distance and direction vector
	if (fCurrentDistance > 1) // If it's not really close
	{
		vecSub(pNode->m_afPosition, vfDirection, pNode->m_afPosition); // Move the node position a single unit in direction
	}
}

void moveToWorldOrderPositions(raaNode *pNode)
{
	moveToSortedOrder(pNode->m_afWorldOrderPosition, pNode);
}

void moveToContinentPositions(raaNode *pNode)
{
	moveToSortedOrder(pNode->m_afContinentPosition, pNode);
}
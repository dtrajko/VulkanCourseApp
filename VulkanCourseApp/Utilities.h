#pragma once

// Indices (locations) of Queue Families (if they exist at all)

struct QueueFamilyIndices
{
	int graphicsFamily = -1;    // Location of Graphics Queue Family

	// Check if Queue Families are valid
	bool isValid()
	{
		return graphicsFamily >= 0;
	}
};

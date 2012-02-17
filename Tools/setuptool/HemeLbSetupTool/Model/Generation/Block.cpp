#include "Index.h"

#include "Site.h"
#include "Block.h"
#include "Domain.h"


/*
 * Helper functions to check if sites are on the edge of the Domain.
 */
bool _CheckMin(const Index& ind) {
	if (ind[0] == 0 || ind[1] == 0 || ind[2] == 0)
		return true;
	return false;
}
bool _CheckMax(const Index& ind, const Index &max) {
	if (ind[0] == max[0] || ind[1] == max[1] || ind[2] == max[2])
		return true;
	return false;
}
bool BlockHasEdges(const Block& block) {
	const Index& ind = block.GetIndex();
	if (_CheckMin(ind))
		return true;

	if (_CheckMax(ind, block.GetDomain().GetBlockCounts()))
		return true;
	return false;
}
bool SiteIsEdge(const Site& site) {
	const Index& ind = site.GetIndex();
	if (_CheckMin(ind))
		return true;

	if (_CheckMax(ind, site.GetBlock().GetDomain().GetSiteCounts()))
		return true;
	return false;
}


Block::Block(Domain& dom, const Index& ind, const unsigned int& size) :
	size(size), index(ind), min(ind * size), max((ind + 1) * size), domain(dom) {
	this->sites.resize(size * size * size);
	unsigned int ijk = 0;
	const bool blockHasEdge = BlockHasEdges(*this);
	Site* site = NULL;

	for (unsigned int i = ind[0] * size; i < (ind[0] + 1) * size; ++i) {
		for (unsigned int j = ind[1] * size; j < (ind[1] + 1) * size; ++j) {
			for (unsigned int k = ind[2] * size; k < (ind[2] + 1) * size; ++k) {
				this->sites[ijk] = site = new Site(*this, i, j, k);

				/*
				 * If the site is on the edge of the domain, we known that it
				 * must be solid. Set this here in order to bootstrap the
				 * classification process.
				 */
				if (blockHasEdge && SiteIsEdge(*site)) {
					site->IsFluidKnown = true;
					site->IsFluid = false;
				}

				++ijk;
			}
		}
	}
}

Block::~Block() {
	SiteIterator end = this->sites.end();
	SiteIterator current = this->sites.begin();
	for (; current != end; ++current) {
		// current will dereference to a Site*
		delete *current;
	}
}

Site& Block::GetGlobalSite(const Index& globalInd) {
	bool local = true;
	for (unsigned int i = 0; i < 3; ++i) {
		if (globalInd[i] < this->min[i] || globalInd[i] >= this->max[i]) {
			local = false;
			break;
		}
	}
	if (local)
		return this->GetLocalSite(globalInd - this->min);

	// Check if the coords belong to another block, i.e. any of
	// the local ones outside the range [0, self.size)

	return this->domain.GetSite(globalInd);
}

Site& Block::GetLocalSite(const Index& ind) {
	/*
	 * Get the site, creating it if it didn't exist.
	 */
	unsigned int ijk = this->TranslateIndex(ind);
	return *this->sites[ijk];
}

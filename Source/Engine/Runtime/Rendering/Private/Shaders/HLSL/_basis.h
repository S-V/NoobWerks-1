// routines for constructing an orthonormal basis with arbitrary tangent and bitangent

#ifndef __NW_SHADERS_BASIS_H__
#define __NW_SHADERS_BASIS_H__

#define nwLOCAL_RIGHT	float3(1,0,0)
#define nwLOCAL_UP		float3(0,0,1)


// Stolen from Matthias Moulin's MAGE:
// https://github.com/matt77hias/MAGE/blob/8c5e926e83f4e7fadab691b08d1551318c04603b/MAGE/Shaders/shaders/basis.hlsli

/**
 Calculates an orthonormal basis from a given unit vector with the method of
 Hughes and Moeller.
 @pre			@a n is normalized.
 @param[in]		n
				A basis vector of the orthonormal basis.
 @return		An orthonormal basis (i.e. object-to-world transformation).
 */
float3x3 buildOrthonormalBasisFromNormal_HughesMoller(float3 n)
{
	const float3 abs_n   = abs(n);
	const float3 n_ortho = (abs_n.x > abs_n.z) ? float3(-n.y,  n.x, 0.0f)
		                                       : float3(1.0f, -n.z,  n.y);
	const float3 t = normalize(cross(n_ortho, n));
	const float3 b = cross(n, t);

	return float3x3(t, b, n);
}

/**
 Calculates an orthonormal basis from a given unit vector with the method of
 Duff, Burgess, Christensen, Hery, Kensler, Liani and Villemin.
 
 See "Building an Orthonormal Basis, Revisited - Pixar Graphics Technologies" [2017]
 https://graphics.pixar.com/library/OrthonormalB/paper.pdf

 @pre			@a n is normalized.
 @param[in]		n
				A basis vector of the orthonormal basis.
 @return		An orthonormal basis (i.e. object-to-world transformation).
 */
float3x3 buildOrthonormalBasisFromNormal_Duff(float3 n)
{
	const float  s  = (0.0f > n.z) ? -1.0f : 1.0f;
	const float  a0 = -1.0f / (s + n.z);
	const float  a1 = n.x * n.y * a0;

	const float3 t  = { 1.0f + s * n.x * n.x * a0, s * a1, -s * n.x };
	const float3 b  = { a1, s + n.y * n.y * a0, -n.y };

	return float3x3(t, b, n);
}

/**
 Calculates an orthonormal basis from a given unit vector with the method.
 @pre			@a n is normalized.
 @param[in]		n
				A basis vector of the orthonormal basis.
 @return		An orthonormal basis (i.e. object-to-world transformation).
 */
float3x3 buildOrthonormalBasisFromNormal(float3 n)
{
	return buildOrthonormalBasisFromNormal_Duff(n);
}

#endif // __NW_SHADERS_BASIS_H__

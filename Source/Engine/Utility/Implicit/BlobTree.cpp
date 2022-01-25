#include "Base/Base.h"
#pragma hdrstop
#include <Scripting/Lua/FunctionBinding.h>
#include <Utility/Implicit/BlobTree.h>

namespace implicit {

BlobTree::BlobTree()
{
	_nodePool.Initialize(sizeof(Node),16);
}


BlobTreeBuilder::BlobTreeBuilder()
{

}

BlobTreeBuilder::~BlobTreeBuilder()
{

}

ERet BlobTreeBuilder::Initialize()
{
	_node_pool.Initialize( sizeof(Node), 16 );
	return ALL_OK;
}

void BlobTreeBuilder::Shutdown()
{
	_node_pool.releaseMemory();
}

void BlobTreeBuilder::bindToLua( lua_State * L )
{
	struct Funcs
	{
		static int op_difference(lua_State * L)
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 2);

			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();
			//luaL_checkudata()
			Node* arg1 = (Node*) lua_touserdata(L, 1);
			Node* arg2 = (Node*) lua_touserdata(L, 2);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_DIFFERENCE;
			newnode->binary.left_operand = arg1;
			newnode->binary.right_operand = arg2;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}

		static int op_intersection(lua_State * L)
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 2);

			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();
			//luaL_checkudata()
			Node* arg1 = (Node*) lua_touserdata(L, 1);
			Node* arg2 = (Node*) lua_touserdata(L, 2);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_INTERSECTION;
			newnode->binary.left_operand = arg1;
			newnode->binary.right_operand = arg2;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}

		static int op_union(lua_State * L)
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 2);

			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();
			//luaL_checkudata()
			Node* arg1 = (Node*) lua_touserdata(L, 1);
			Node* arg2 = (Node*) lua_touserdata(L, 2);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_UNION;
			newnode->binary.left_operand = arg1;
			newnode->binary.right_operand = arg2;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}

		static void* plane(double a, double b, double c, double d, int matid)
		{
			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_PLANE;
			newnode->plane.normal_and_distance.x = a;
			newnode->plane.normal_and_distance.y = b;
			newnode->plane.normal_and_distance.z = c;
			newnode->plane.normal_and_distance.w = d;
			newnode->plane.material_id = matid;

			return newnode;
		}

		// centerX, centerY, centerZ, radiusX, materialId
		static int ball(lua_State * L)
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 5);

			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();

			double centerx = luaL_checknumber(L, 1);
			double centery = luaL_checknumber(L, 2);
			double centerz = luaL_checknumber(L, 3);
			double radius = luaL_checknumber(L, 4);
			int materialid = luaL_checkinteger(L, 5);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_SPHERE;
			newnode->sphere.center = CV3f(centerx, centery, centerz);
			newnode->sphere.radius = radius;
			newnode->sphere.material_id = materialid;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}

		// box(centerX, centerY, centerZ, halfSizeX, halfSizeY, halfSizeZ, materialId)
		static int box(lua_State * L)
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 7);

			//luaL_checktype(L, 1, LUA_TUSERDATA);
			//BlobTreeBuilder* treeBuilder = (BlobTreeBuilder*) lua_touserdata(L, 1);
			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();

			double centerx = luaL_checknumber(L, 1);
			double centery = luaL_checknumber(L, 2);
			double centerz = luaL_checknumber(L, 3);
			double extentx = luaL_checknumber(L, 4);
			double extenty = luaL_checknumber(L, 5);
			double extentz = luaL_checknumber(L, 6);
			int materialid = luaL_checkinteger(L, 7);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_BOX;
			newnode->box.center = CV3f(centerx, centery, centerz);
			newnode->box.extent = CV3f(extentx, extenty, extentz);
			newnode->box.material_id = materialid;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}

		static int infinite_cylinder_z( lua_State * L )
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 4);

			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();

			double centerx = luaL_checknumber(L, 1);
			double centery = luaL_checknumber(L, 2);
			double radius = luaL_checknumber(L, 3);
			int materialid = luaL_checkinteger(L, 4);

			mxASSERT(radius > 0);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_INF_CYLINDER;
			newnode->inf_cylinder.origin = CV3f(centerx, centery, 0);
			newnode->inf_cylinder.radius = radius;
			newnode->inf_cylinder.material_id = materialid;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}

		static int torus( lua_State * L )
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 6);

			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();

			double centerx = luaL_checknumber(L, 1);
			double centery = luaL_checknumber(L, 2);
			double centerz = luaL_checknumber(L, 3);
			double outer_radius = luaL_checknumber(L, 4);
			double inner_radius = luaL_checknumber(L, 5);
			int materialid = luaL_checkinteger(L, 6);

			mxASSERT(inner_radius > 0 && inner_radius < outer_radius);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_TORUS;
			newnode->torus.center = CV3f(centerx, centery, centerz);
			newnode->torus.big_radius = outer_radius;
			newnode->torus.small_radius = inner_radius;
			newnode->torus.material_id = materialid;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}

		static int sine_wave( lua_State * L )
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 2);

			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();

			double scale = luaL_checknumber(L, 1);
			int materialid = luaL_checkinteger(L, 2);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_SINE_WAVE;
			newnode->sine_wave.coord_scale = CV3f(scale);
			newnode->sine_wave.material_id = materialid;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}

		static int gyroid(lua_State * L)
		{
			const int num_arguments = lua_gettop(L);
			mxASSERT(num_arguments == 2);

			BlobTreeBuilder& treeBuilder = BlobTreeBuilder::Get();

			double scale = luaL_checknumber(L, 1);
			int materialid = luaL_checkinteger(L, 2);

			Node* newnode = treeBuilder._node_pool.New<Node>();

			newnode->type = NT_GYROID;
			newnode->gyroid.coord_scale = CV3f(scale);
			newnode->gyroid.material_id = materialid;

			lua_pushlightuserdata(L, newnode);
			return 1;
		}
	};

	lua_pushcfunction(L, &Funcs::op_intersection);
	lua_setglobal(L, "I");

	lua_pushcfunction(L, &Funcs::op_difference);
	lua_setglobal(L, "D");

	lua_pushcfunction(L, &Funcs::op_union);
	lua_setglobal(L, "U");


	lua_register( L, "plane", TO_LUA_CALLBACK(Funcs::plane) );
	//lua_register( L, "ball", TO_LUA_CALLBACK(Funcs::ball) );
	//lua_register( L, "gyroid", TO_LUA_CALLBACK(Funcs::gyroid) );
	//lua_register( L, "box", TO_LUA_CALLBACK(Funcs::box) );
	lua_pushcfunction(L, &Funcs::box);
	lua_setglobal(L, "box");

	lua_pushcfunction(L, &Funcs::ball);
	lua_setglobal(L, "ball");

	lua_pushcfunction(L, &Funcs::infinite_cylinder_z);
	lua_setglobal(L, "inf_cyl_z");

	lua_pushcfunction(L, &Funcs::torus);
	lua_setglobal(L, "torus");

	lua_pushcfunction(L, &Funcs::sine_wave);
	lua_setglobal(L, "sine_wave");

	lua_pushcfunction(L, &Funcs::gyroid);
	lua_setglobal(L, "gyroid");
}

void BlobTreeBuilder::unbindFromLua( lua_State * L )
{
	mxTODO("")
}

}//namespace implicit

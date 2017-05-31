#include <array>
#include <vector>
#include <memory>

#include <iostream>

/*
QuadTree is a data structure to divide 2D region to more manageable parts.
It's as a binary tree but it has four children nodes.

II  |  I
---------
III | IV

Information:
That implementation is adapted to work with SFML library, but
can be used with others also (see requirements below).
T - parameter is a type of objects that will be kept in the tree node.
Bound - parameter that appoint bound of each tree node.

Requirements:
Type T must have public functions:
 >> getPosition() - that returns actual position of the object and it has public variables
					x, y that indicates actual x - axis, and y - axis position of that object.
 >> getSize()	  - that returns actual size of the object and it has public variables
					x, y that indicates actual width and height respectively.

Type Bound must have public variables x, y representing their position on x, y axis respectively.
*/
template <typename T, typename Bound>
class QuadTree
{
public:
	// QTree_uPtr is a unique_ptr to the QuadTree<T> where T is a type of object
	using QTree_uPtr = std::unique_ptr<QuadTree<T, Bound>>;

	// Obj_uPtr is a unique_ptr to object held in tree
	using Obj_uPtr = std::unique_ptr<T>;


	// Constructor
	QuadTree(uint32_t pLevel, Bound top_left, Bound bottom_right, uint32_t max_objects, uint32_t max_level);

	// Clears whole tree from the current.
	void clear();

	// Insert the object to the tree.
	void insert(const T& object);

	// Returns elements that can actually collide with the 'object'.
	std::vector<const T*>& retrieve(std::vector<const T*>& obj_vec, const T& object);

private:
	// Array of pointers to child node.
	std::array<QTree_uPtr, 4> children_nodes;

	// Pointers to objects held in current node
	std::vector<const T*> node_objects;

	// Bound of top left corner
	Bound top_left;

	// Bound of bottom right corner
	Bound bottom_right;

	// Maximum number of objects that can be hold in that node
	uint32_t max_objects;

	// Actual node level
	uint32_t level;

	// Max available level
	uint32_t max_level;

private:
	// Split the tree to four sub-tree.
	void split();

	// Checks in which quarter of coordinate system the object is
	int getIndex(const T& object);

	// Checks if the object position fits inside the tree
	bool is_inside(const T& object);
};

template<typename T, typename Bound>
inline QuadTree<T, Bound>::QuadTree(uint32_t pLevel, Bound top_left, Bound bottom_right, uint32_t max_objects, uint32_t max_level)
	: children_nodes(), node_objects(), top_left(top_left), bottom_right(bottom_right), 
	max_objects(max_objects), level(pLevel), max_level(max_level)
{
	children_nodes[0] = nullptr;
	children_nodes[1] = nullptr;
	children_nodes[2] = nullptr;
	children_nodes[3] = nullptr;
}

template<typename T, typename Bound>
inline void QuadTree<T, Bound>::clear()
{
	// Clear current
	node_objects.clear();

	// Clear children sub-trees
	for (int i = 0; i < children_nodes.size(); ++i)
		if (children_nodes[i] != nullptr)
		{
			children_nodes[i]->clear();
			children_nodes[i] = nullptr;
		}
}

template<typename T, typename Bound>
inline void QuadTree<T, Bound>::insert(const T& object)
{
	// Current is not leaf -> Check Children
	if (children_nodes[0] != nullptr)
	{
		for (int i = 0; i < 4; i++)
			if (children_nodes[i]->is_inside(object))
				children_nodes[i]->insert(object);
		return;
	}

	// Is leaf -> push here
	node_objects.push_back(&object);

	// Check if should split
	if (node_objects.size() > max_objects && level < max_level)
	{
		if (children_nodes[0] == nullptr)
			split();

		while (node_objects.size())
		{
			auto tmpObj = node_objects[node_objects.size() - 1];
			node_objects.pop_back();
			for (int j = 0; j < 4; j++)
				if (children_nodes[j]->is_inside(*tmpObj))
					children_nodes[j]->node_objects.push_back(tmpObj);
		}
	}
}

template<typename T, typename Bound>
inline std::vector<const T*>& QuadTree<T, Bound>::retrieve(std::vector<const T*>& obj_vec, const T& object)
{
	// Is leaf -> push objects from that area
	if (children_nodes[0] == nullptr)
		for (const T* o : node_objects)
			obj_vec.push_back(o);
	// Is not leaf -> check sub-trees
	else
		for (int i = 0; i < 4; i++)
			if (children_nodes[i]->is_inside(object))
				children_nodes[i]->retrieve(obj_vec, object);

	return obj_vec;
}

template<typename T, typename Bound>
inline void QuadTree<T, Bound>::split()
{
	auto hf_width = (bottom_right.x - top_left.x) / 2;
	auto hf_height = (bottom_right.y - top_left.y) / 2;

	children_nodes[0] = std::make_unique<QuadTree>(level + 1,
		Bound(top_left.x + hf_width, top_left.y), Bound(bottom_right.x, top_left.y + hf_height), max_objects, max_level);

	children_nodes[1] = std::make_unique<QuadTree>(level + 1,
		Bound(top_left.x, top_left.y), Bound(top_left.x + hf_width, top_left.y + hf_height), max_objects, max_level);

	children_nodes[2] = std::make_unique<QuadTree>(level + 1,
		Bound(top_left.x, top_left.y + hf_height), Bound(top_left.x + hf_width, bottom_right.y), max_objects, max_level);

	children_nodes[3] = std::make_unique<QuadTree>(level + 1,
		Bound(top_left.x + hf_width, top_left.y + hf_height), Bound(bottom_right.x, bottom_right.y), max_objects, max_level);
}

template<typename T, typename Bound>
inline int QuadTree<T, Bound>::getIndex(const T& object)
{
	int index = -1;
	auto v_midPnt = top_left.x + (bottom_right.x - top_left.x) / 2;
	auto h_midPnt = top_left.y + (bottom_right.y - top_left.y) / 2;

	// Object can completely fit within the top quadrants
	bool topQuadrant = (object.getPosition().y < h_midPnt && object.getPosition().y + object.getSize().y < h_midPnt);

	// Object can completely fit within the bottom quadrants
	bool bottomQuadrant = (object.getPosition().y > h_midPnt);

	// Object can completely fit within the left quadrants
	if (object.getPosition().x < v_midPnt && object.getPosition().x + object.getSize().x < v_midPnt)
	{
		if (topQuadrant)
			index = 1;
		else if (bottomQuadrant)
			index = 2;
	}
	// Object can completely fit within the right quadrants
	else if (object.getPosition().x > v_midPnt)
	{
		if (topQuadrant)
			index = 0;
		else if (bottomQuadrant)
			index = 3;
	}

	return index;
}

template<typename T, typename Bound>
inline bool QuadTree<T, Bound>::is_inside(const T& object)
{
	return object.getPosition().x + object.getSize().x >= top_left.x &&
		object.getPosition().x <= bottom_right.x &&
		object.getPosition().y + object.getSize().y >= top_left.y &&
		object.getPosition().y <= bottom_right.y;
}
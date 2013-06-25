#ifndef VISITOR_HPP
#define VISITOR_HPP

class Box_portal;

class Plane_barrier;
class Box_barrier;
class Blow_barrier;
class Moving_box_barrier;

class Molecule_releaser;

class Brownian_box;
class Brownian_plane;

class Level_element_visitor
{
public:
    virtual void visit(Plane_barrier *) const {}
    virtual void visit(Box_barrier *) const {}
    virtual void visit(Blow_barrier *) const {}
    virtual void visit(Moving_box_barrier *) const {}

    virtual void visit(Molecule_releaser *) const {}

    virtual void visit(Box_portal *) const {}

    virtual void visit(Brownian_box *) const {}
    virtual void visit(Brownian_plane *) const {}
};

#endif // VISITOR_HPP
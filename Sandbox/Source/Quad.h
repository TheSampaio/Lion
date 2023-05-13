#ifndef SANDBOX_QUAD_H
#define SANDBOX_QUAD_H

class Quad : public Entity
{
public:
	Quad();
	virtual ~Quad();

	virtual void Start();
	virtual void Update(float DeltaTime);
	virtual void Draw();

private:
	Mesh* StaticMesh;
};

#endif // !SANDBOX_QUAD_H


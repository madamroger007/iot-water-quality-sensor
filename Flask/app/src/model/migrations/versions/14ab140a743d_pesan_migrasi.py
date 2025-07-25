"""Pesan Migrasi

Revision ID: 14ab140a743d
Revises: 
Create Date: 2025-06-06 00:38:58.840206

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = '14ab140a743d'
down_revision = None
branch_labels = None
depends_on = None


def upgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.create_table('data_sensor',
    sa.Column('id', sa.Integer(), nullable=False),
    sa.Column('ph', sa.Float(), nullable=True),
    sa.Column('tds', sa.Float(), nullable=True),
    sa.Column('suhu', sa.Float(), nullable=True),
    sa.Column('turbidity', sa.Float(), nullable=True),
    sa.Column('kelayakan', sa.Boolean(), nullable=True),
    sa.Column('dibuat_sejak', sa.DateTime(timezone=True), nullable=True),
    sa.PrimaryKeyConstraint('id')
    )
    op.create_table('nomor_hp',
    sa.Column('id', sa.Integer(), nullable=False),
    sa.Column('nomor_hp', sa.String(length=15), nullable=False),
    sa.Column('dibuat_sejak', sa.DateTime(timezone=True), nullable=True),
    sa.Column('diubah_sejak', sa.DateTime(timezone=True), nullable=True),
    sa.PrimaryKeyConstraint('id'),
    sa.UniqueConstraint('nomor_hp')
    )
    # ### end Alembic commands ###


def downgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_table('nomor_hp')
    op.drop_table('data_sensor')
    # ### end Alembic commands ###
